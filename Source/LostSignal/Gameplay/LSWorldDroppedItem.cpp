#include "Gameplay/LSWorldDroppedItem.h"

#include "Components/WidgetComponent.h"
#include "Data/LSArmorRow.h"
#include "Data/LSChipRow.h"
#include "Data/LSDropSettings.h"
#include "Data/LSItemRow.h"
#include "Data/LSWeaponRow.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "LostSignal.h"
#include "Net/UnrealNetwork.h"
#include "Session/LSSessionSubsystem.h"
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

	InteractText = NSLOCTEXT("LSWorldDroppedItem", "PickUpInteractText", "Pick up");
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

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (ULSSessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<ULSSessionSubsystem>())
		{
			FLSSessionItem RemainingItem;
			if (!SessionSubsystem->TryAddSessionItem(ItemRowName, Amount, RemainingItem))
			{
				return;
			}

			if (RemainingItem.ItemRowName.IsNone() || RemainingItem.Amount <= 0)
			{
				Destroy();
			}
			else
			{
				ItemRowName = RemainingItem.ItemRowName;
				Amount = RemainingItem.Amount;
				RefreshItemVisual();
				ForceNetUpdate();
			}
		}
		else
		{
			UE_LOG(LogLS, Warning, TEXT("Cannot pick up dropped item because SessionSubsystem is missing on %s."), *GetNameSafe(this));
		}
	}
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
}

void ALSWorldDroppedItem::InitializeDroppedItem(const FLSSessionItem& InItem)
{
	ItemRowName = InItem.ItemRowName;
	Amount = InItem.Amount;
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
	const ULSDropSettings* Settings = GetDefault<ULSDropSettings>();
	if (!Settings)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot load dropped item icon because LS Drop Settings is missing."));
		return nullptr;
	}

	const FString RowNameString = InItemRowName.ToString();
	FString IconNameOrPath;

	if (RowNameString.StartsWith(TEXT("Chip_")))
	{
		UDataTable* ChipTable = Settings->ChipTable.LoadSynchronous();
		const FLSChipRow* Row = ChipTable ? ChipTable->FindRow<FLSChipRow>(InItemRowName, TEXT("LoadDroppedItemIconTextureByRowName")) : nullptr;
		IconNameOrPath = Row ? Row->Icon_Path : FString();
	}
	else if (RowNameString.StartsWith(TEXT("Weapon_")))
	{
		UDataTable* WeaponTable = Settings->WeaponTable.LoadSynchronous();
		const FLSWeaponRow* Row = WeaponTable ? WeaponTable->FindRow<FLSWeaponRow>(InItemRowName, TEXT("LoadDroppedItemIconTextureByRowName")) : nullptr;
		IconNameOrPath = Row ? Row->Icon_Path : FString();
	}
	else if (RowNameString.StartsWith(TEXT("Armor_")))
	{
		UDataTable* ArmorTable = Settings->ArmorTable.LoadSynchronous();
		const FLSArmorRow* Row = ArmorTable ? ArmorTable->FindRow<FLSArmorRow>(InItemRowName, TEXT("LoadDroppedItemIconTextureByRowName")) : nullptr;
		IconNameOrPath = Row ? Row->Icon_Path : FString();
	}
	else if (RowNameString.StartsWith(TEXT("Item_")))
	{
		UDataTable* ItemTable = Settings->ItemTable.LoadSynchronous();
		const FLSItemRow* Row = ItemTable ? ItemTable->FindRow<FLSItemRow>(InItemRowName, TEXT("LoadDroppedItemIconTextureByRowName")) : nullptr;
		IconNameOrPath = Row ? Row->Icon_Path : FString();
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot load dropped item icon because row '%s' has an unknown prefix."), *InItemRowName.ToString());
		return nullptr;
	}

	if (IconNameOrPath.IsEmpty())
	{
		IconNameOrPath = InItemRowName.ToString();
	}

	const FString IconObjectPath = BuildIconObjectPath(IconNameOrPath, GetIconBaseFolderByRowName(InItemRowName));
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
