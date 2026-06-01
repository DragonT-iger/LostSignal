#include "Gameplay/LSWorldDroppedItem.h"

#include "Components/WidgetComponent.h"
#include "Core/LSPlayerControllerBase.h"
#include "Engine/Texture2D.h"
#include "Inventory/LSRaidInventoryComponent.h"
#include "LostSignal.h"
#include "Net/UnrealNetwork.h"
#include "Session/LSSaveSubsystem.h"
#include "UI/Inventory/LSWorldDroppedItemIconWidget.h"

ALSWorldDroppedItem::ALSWorldDroppedItem()
{
	ItemIconWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("ItemIconWidget"));
	ItemIconWidgetComponent->SetupAttachment(RootComponent);
	ItemIconWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ItemIconWidgetComponent->SetGenerateOverlapEvents(false);
	ItemIconWidgetComponent->SetWidgetSpace(EWidgetSpace::World);
	ItemIconWidgetComponent->SetWidgetClass(ULSWorldDroppedItemIconWidget::StaticClass());
	ItemIconWidgetComponent->SetDrawSize(IconDrawSize);
	ItemIconWidgetComponent->SetTwoSided(true);
	ItemIconWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, GroundOffsetZ));
	ItemIconWidgetComponent->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
}

void ALSWorldDroppedItem::BeginPlay()
{
	Super::BeginPlay();
	RefreshItemVisual();
}

bool ALSWorldDroppedItem::CanInteract_Implementation(APawn* Interactor)
{
	return !ItemRowName.IsNone() && Amount > 0;
}

void ALSWorldDroppedItem::Interact_Implementation(APawn* Interactor)
{
	if (!HasAuthority() || ItemRowName.IsNone() || Amount <= 0)
	{
		return;
	}

	ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(Interactor ? Interactor->GetController() : nullptr);
	ULSRaidInventoryComponent* RaidInventory = PlayerController ? PlayerController->GetRaidInventoryComponent() : nullptr;
	FLSSessionItem RemainingItem;
	if (RaidInventory && RaidInventory->IsRaidActive())
	{
		if (!RaidInventory->TryAddSessionItem(ItemRowName, Amount, StatSeed, RemainingItem))
		{
			return;
		}

		PlayerController->SyncRaidInventoryToClient();
	}
	else
	{
		UGameInstance* GameInstance = GetGameInstance();
		ULSSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
		if (!SaveSubsystem || !SaveSubsystem->TryAddToInventory(ItemRowName, Amount, StatSeed, RemainingItem))
		{
			UE_LOG(LogLS, Warning, TEXT("Cannot pick up dropped item because no inventory storage is available on %s."), *GetNameSafe(this));
			return;
		}
	}

	if (RemainingItem.ItemRowName.IsNone() || RemainingItem.Amount <= 0)
	{
		Destroy();
		return;
	}

	ItemRowName = RemainingItem.ItemRowName;
	Amount = RemainingItem.Amount;
	StatSeed = RemainingItem.StatSeed;
	RefreshItemVisual();
	ForceNetUpdate();
}

FText ALSWorldDroppedItem::GetInteractText_Implementation()
{
	return InteractText;
}

void ALSWorldDroppedItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALSWorldDroppedItem, ItemRowName);
	DOREPLIFETIME(ALSWorldDroppedItem, Amount);
	DOREPLIFETIME(ALSWorldDroppedItem, StatSeed);
}

void ALSWorldDroppedItem::InitializeDroppedItem(const FLSSessionItem& InItem)
{
	ItemRowName = InItem.ItemRowName;
	Amount = InItem.Amount;
	StatSeed = InItem.StatSeed;
	RefreshItemVisual();
}

void ALSWorldDroppedItem::OnRep_ItemData()
{
	RefreshItemVisual();
}

void ALSWorldDroppedItem::RefreshItemVisual()
{
	if (!ItemIconWidgetComponent)
	{
		UE_LOG(LogLS, Warning, TEXT("ItemIconWidgetComponent is not bound on %s."), *GetNameSafe(this));
		return;
	}

	ItemIconWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, GroundOffsetZ));
	ItemIconWidgetComponent->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	ItemIconWidgetComponent->SetDrawSize(IconDrawSize);
	ItemIconWidgetComponent->InitWidget();

	UTexture2D* IconTexture = LoadIconTextureByRowName(ItemRowName);
	if (!IconTexture)
	{
		IconTexture = LoadDefaultIconTexture();
		UE_LOG(LogLS, Warning, TEXT("Using default dropped item icon for row '%s' on %s."), *ItemRowName.ToString(), *GetNameSafe(this));
	}

	if (ULSWorldDroppedItemIconWidget* IconWidget = Cast<ULSWorldDroppedItemIconWidget>(ItemIconWidgetComponent->GetWidget()))
	{
		IconWidget->SetIconTexture(IconTexture);
		return;
	}

	UE_LOG(LogLS, Warning, TEXT("Dropped item icon widget is missing or invalid on %s."), *GetNameSafe(this));
}

UTexture2D* ALSWorldDroppedItem::LoadIconTextureByRowName(const FName InItemRowName) const
{
	const FString IconObjectPath = BuildIconObjectPath(InItemRowName.ToString(), GetIconBaseFolderByRowName(InItemRowName));
	UTexture2D* IconTexture = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, *IconObjectPath));
	if (!IconTexture)
	{
		UE_LOG(LogLS, Warning, TEXT("Failed to load dropped item icon '%s' for row '%s'."), *IconObjectPath, *InItemRowName.ToString());
	}

	return IconTexture;
}

UTexture2D* ALSWorldDroppedItem::LoadDefaultIconTexture() const
{
	static const TCHAR* DefaultIconObjectPath = TEXT("/Engine/EngineResources/DefaultTexture.DefaultTexture");
	UTexture2D* DefaultIconTexture = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, DefaultIconObjectPath));
	if (!DefaultIconTexture)
	{
		UE_LOG(LogLS, Warning, TEXT("Failed to load default dropped item icon '%s'."), DefaultIconObjectPath);
	}

	return DefaultIconTexture;
}

FString ALSWorldDroppedItem::BuildIconObjectPath(const FString& IconNameOrPath, const FString& BaseFolder)
{
	if (IconNameOrPath.StartsWith(TEXT("/Game/")))
	{
		if (IconNameOrPath.Contains(TEXT(".")))
		{
			return IconNameOrPath;
		}

		FString AssetName;
		IconNameOrPath.Split(TEXT("/"), nullptr, &AssetName, ESearchCase::CaseSensitive, ESearchDir::FromEnd);
		return FString::Printf(TEXT("%s.%s"), *IconNameOrPath, *AssetName);
	}

	return FString::Printf(TEXT("%s%s.%s"), *BaseFolder, *IconNameOrPath, *IconNameOrPath);
}

FString ALSWorldDroppedItem::GetIconBaseFolderByRowName(const FName InItemRowName)
{
	const FString RowNameString = InItemRowName.ToString();
	if (RowNameString.StartsWith(TEXT("Chip_")))
	{
		return TEXT("/Game/LostSignal/UI/Icons/Chips/");
	}

	if (RowNameString.StartsWith(TEXT("Weapon_")))
	{
		return TEXT("/Game/LostSignal/UI/Icons/Weapons/");
	}

	if (RowNameString.StartsWith(TEXT("Armor_")))
	{
		return TEXT("/Game/LostSignal/UI/Icons/Armors/");
	}

	return TEXT("/Game/LostSignal/UI/Icons/Items/");
}
