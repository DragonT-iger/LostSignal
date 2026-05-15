#include "UI/Storage/LSLobbyStorageWidget.h"

#include "Components/TextBlock.h"
#include "Components/WrapBox.h"
#include "Data/LSDropSettings.h"
#include "Data/LSItemRow.h"
#include "Engine/DataTable.h"
#include "LostSignal.h"
#include "Session/LSSaveSubsystem.h"
#include "UI/Inventory/LSItemSlotWidget.h"
#include "UI/Storage/LSStorageButtonWidget.h"

#define LOCTEXT_NAMESPACE "LSLobbyStorageWidget"

void ULSLobbyStorageWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!StorageSlotWrapBox)
	{
		UE_LOG(LogLS, Warning, TEXT("StorageSlotWrapBox is not bound on %s."), *GetNameSafe(this));
	}

	if (!StorageCountText)
	{
		UE_LOG(LogLS, Warning, TEXT("StorageCountText is not bound on %s."), *GetNameSafe(this));
	}

	if (!ItemSlotWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("ItemSlotWidgetClass is not set on %s."), *GetNameSafe(this));
	}

	BindStorageButtons();
	ApplyFilterButtonState();
	RefreshStorage();
}

void ULSLobbyStorageWidget::NativeDestruct()
{
	UnbindStorageButtons();

	Super::NativeDestruct();
}

void ULSLobbyStorageWidget::SetFilter(const ELSStorageFilter NewFilter)
{
	CurrentFilter = NewFilter;
	ApplyFilterButtonState();
	RefreshStorage();
}

void ULSLobbyStorageWidget::RefreshStorage()
{
	if (!StorageSlotWrapBox)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot refresh lobby storage because StorageSlotWrapBox is not bound on %s."), *GetNameSafe(this));
		return;
	}

	StorageSlotWrapBox->ClearChildren();

	ULSSaveSubsystem* SaveSubsystem = GetSaveSubsystem();
	static const TArray<FLSSessionItem> EmptyStashItems;
	const TArray<FLSSessionItem>& StashItems = SaveSubsystem ? SaveSubsystem->GetStash() : EmptyStashItems;
	UpdateStorageCountText(StashItems);

	if (!SaveSubsystem)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot refresh lobby storage because SaveSubsystem is missing on %s."), *GetNameSafe(this));
		return;
	}

	if (!ItemSlotWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot refresh lobby storage because ItemSlotWidgetClass is not set on %s."), *GetNameSafe(this));
		return;
	}

	APlayerController* OwningPlayer = GetOwningPlayer();
	UWorld* World = GetWorld();
	if (!OwningPlayer && !World)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot refresh lobby storage because owner/world is missing on %s."), *GetNameSafe(this));
		return;
	}

	TArray<FLSSessionItem> FilteredItems;
	BuildFilteredItems(StashItems, FilteredItems);

	const int32 SlotCountToBuild = FMath::Max(0, MaxStorageSlotCount);
	if (FilteredItems.Num() > SlotCountToBuild)
	{
		UE_LOG(LogLS, Warning, TEXT("Lobby storage has %d filtered items but only %d slots are visible on %s."),
			FilteredItems.Num(),
			SlotCountToBuild,
			*GetNameSafe(this));
	}

	for (int32 SlotIndex = 0; SlotIndex < SlotCountToBuild; ++SlotIndex)
	{
		ULSItemSlotWidget* SlotWidget = OwningPlayer
			? CreateWidget<ULSItemSlotWidget>(OwningPlayer, ItemSlotWidgetClass)
			: CreateWidget<ULSItemSlotWidget>(World, ItemSlotWidgetClass);

		if (!SlotWidget)
		{
			UE_LOG(LogLS, Warning, TEXT("Failed to create lobby storage slot widget at index %d on %s."), SlotIndex, *GetNameSafe(this));
			continue;
		}

		if (FilteredItems.IsValidIndex(SlotIndex) && IsFilledStorageSlot(FilteredItems[SlotIndex]))
		{
			SlotWidget->SetItem(FilteredItems[SlotIndex].ItemRowName, FilteredItems[SlotIndex].Amount);
		}
		else
		{
			SlotWidget->ClearItem();
		}

		StorageSlotWrapBox->AddChildToWrapBox(SlotWidget);
	}
}

void ULSLobbyStorageWidget::HandleSortButtonClicked()
{
	if (ULSSaveSubsystem* SaveSubsystem = GetSaveSubsystem())
	{
		SaveSubsystem->SortStash();
		RefreshStorage();
		return;
	}

	UE_LOG(LogLS, Warning, TEXT("Cannot sort lobby storage because SaveSubsystem is missing on %s."), *GetNameSafe(this));
}

void ULSLobbyStorageWidget::HandleAllTabButtonClicked()
{
	SetFilter(ELSStorageFilter::All);
}

void ULSLobbyStorageWidget::HandleWeaponTabButtonClicked()
{
	SetFilter(ELSStorageFilter::Weapon);
}

void ULSLobbyStorageWidget::HandleArmorTabButtonClicked()
{
	SetFilter(ELSStorageFilter::Armor);
}

void ULSLobbyStorageWidget::HandleConsumableTabButtonClicked()
{
	SetFilter(ELSStorageFilter::Consumable);
}

void ULSLobbyStorageWidget::HandleMiscTabButtonClicked()
{
	SetFilter(ELSStorageFilter::Misc);
}

void ULSLobbyStorageWidget::HandleChipTabButtonClicked()
{
	SetFilter(ELSStorageFilter::Chip);
}

void ULSLobbyStorageWidget::BindStorageButtons()
{
	if (!SortButton)
	{
		UE_LOG(LogLS, Warning, TEXT("SortButton is not bound on %s."), *GetNameSafe(this));
	}
	else
	{
		SortButton->OnClicked.AddDynamic(this, &ULSLobbyStorageWidget::HandleSortButtonClicked);
	}

	if (!AllTabButton)
	{
		UE_LOG(LogLS, Warning, TEXT("AllTabButton is not bound on %s."), *GetNameSafe(this));
	}
	else
	{
		AllTabButton->OnClicked.AddDynamic(this, &ULSLobbyStorageWidget::HandleAllTabButtonClicked);
	}

	if (!WeaponTabButton)
	{
		UE_LOG(LogLS, Warning, TEXT("WeaponTabButton is not bound on %s."), *GetNameSafe(this));
	}
	else
	{
		WeaponTabButton->OnClicked.AddDynamic(this, &ULSLobbyStorageWidget::HandleWeaponTabButtonClicked);
	}

	if (!ArmorTabButton)
	{
		UE_LOG(LogLS, Warning, TEXT("ArmorTabButton is not bound on %s."), *GetNameSafe(this));
	}
	else
	{
		ArmorTabButton->OnClicked.AddDynamic(this, &ULSLobbyStorageWidget::HandleArmorTabButtonClicked);
	}

	if (!ConsumableTabButton)
	{
		UE_LOG(LogLS, Warning, TEXT("ConsumableTabButton is not bound on %s."), *GetNameSafe(this));
	}
	else
	{
		ConsumableTabButton->OnClicked.AddDynamic(this, &ULSLobbyStorageWidget::HandleConsumableTabButtonClicked);
	}

	if (!MiscTabButton)
	{
		UE_LOG(LogLS, Warning, TEXT("MiscTabButton is not bound on %s."), *GetNameSafe(this));
	}
	else
	{
		MiscTabButton->OnClicked.AddDynamic(this, &ULSLobbyStorageWidget::HandleMiscTabButtonClicked);
	}

	if (!ChipTabButton)
	{
		UE_LOG(LogLS, Warning, TEXT("ChipTabButton is not bound on %s."), *GetNameSafe(this));
	}
	else
	{
		ChipTabButton->OnClicked.AddDynamic(this, &ULSLobbyStorageWidget::HandleChipTabButtonClicked);
	}
}

void ULSLobbyStorageWidget::UnbindStorageButtons()
{
	if (SortButton)
	{
		SortButton->OnClicked.RemoveDynamic(this, &ULSLobbyStorageWidget::HandleSortButtonClicked);
	}

	if (AllTabButton)
	{
		AllTabButton->OnClicked.RemoveDynamic(this, &ULSLobbyStorageWidget::HandleAllTabButtonClicked);
	}

	if (WeaponTabButton)
	{
		WeaponTabButton->OnClicked.RemoveDynamic(this, &ULSLobbyStorageWidget::HandleWeaponTabButtonClicked);
	}

	if (ArmorTabButton)
	{
		ArmorTabButton->OnClicked.RemoveDynamic(this, &ULSLobbyStorageWidget::HandleArmorTabButtonClicked);
	}

	if (ConsumableTabButton)
	{
		ConsumableTabButton->OnClicked.RemoveDynamic(this, &ULSLobbyStorageWidget::HandleConsumableTabButtonClicked);
	}

	if (MiscTabButton)
	{
		MiscTabButton->OnClicked.RemoveDynamic(this, &ULSLobbyStorageWidget::HandleMiscTabButtonClicked);
	}

	if (ChipTabButton)
	{
		ChipTabButton->OnClicked.RemoveDynamic(this, &ULSLobbyStorageWidget::HandleChipTabButtonClicked);
	}
}

void ULSLobbyStorageWidget::UpdateStorageCountText(const TArray<FLSSessionItem>& StashItems) const
{
	if (!StorageCountText)
	{
		return;
	}

	int32 FilledSlotCount = 0;
	for (const FLSSessionItem& Item : StashItems)
	{
		if (IsFilledStorageSlot(Item))
		{
			++FilledSlotCount;
		}
	}

	StorageCountText->SetText(FText::Format(
		LOCTEXT("StorageCountFormat", "{0}/{1}"),
		FText::AsNumber(FilledSlotCount),
		FText::AsNumber(FMath::Max(0, MaxStorageSlotCount))));
}

void ULSLobbyStorageWidget::ApplyFilterButtonState() const
{
	if (AllTabButton)
	{
		AllTabButton->SetIsEnabled(CurrentFilter != ELSStorageFilter::All);
	}

	if (WeaponTabButton)
	{
		WeaponTabButton->SetIsEnabled(CurrentFilter != ELSStorageFilter::Weapon);
	}

	if (ArmorTabButton)
	{
		ArmorTabButton->SetIsEnabled(CurrentFilter != ELSStorageFilter::Armor);
	}

	if (ConsumableTabButton)
	{
		ConsumableTabButton->SetIsEnabled(CurrentFilter != ELSStorageFilter::Consumable);
	}

	if (MiscTabButton)
	{
		MiscTabButton->SetIsEnabled(CurrentFilter != ELSStorageFilter::Misc);
	}

	if (ChipTabButton)
	{
		ChipTabButton->SetIsEnabled(CurrentFilter != ELSStorageFilter::Chip);
	}
}

void ULSLobbyStorageWidget::BuildFilteredItems(const TArray<FLSSessionItem>& StashItems, TArray<FLSSessionItem>& OutItems) const
{
	OutItems.Reset();

	for (const FLSSessionItem& Item : StashItems)
	{
		if (IsFilledStorageSlot(Item) && DoesItemMatchCurrentFilter(Item.ItemRowName))
		{
			OutItems.Add(Item);
		}
	}
}

bool ULSLobbyStorageWidget::DoesItemMatchCurrentFilter(const FName ItemRowName) const
{
	if (ItemRowName.IsNone())
	{
		return false;
	}

	const FString RowNameString = ItemRowName.ToString();
	switch (CurrentFilter)
	{
	case ELSStorageFilter::All:
		return true;
	case ELSStorageFilter::Weapon:
		return RowNameString.StartsWith(TEXT("Weapon_"));
	case ELSStorageFilter::Armor:
		return RowNameString.StartsWith(TEXT("Armor_"));
	case ELSStorageFilter::Consumable:
		return IsConsumableItem(ItemRowName);
	case ELSStorageFilter::Misc:
		return RowNameString.StartsWith(TEXT("Item_")) && !IsConsumableItem(ItemRowName);
	case ELSStorageFilter::Chip:
		return RowNameString.StartsWith(TEXT("Chip_"));
	default:
		return false;
	}
}

bool ULSLobbyStorageWidget::IsConsumableItem(const FName ItemRowName) const
{
	const FString RowNameString = ItemRowName.ToString();
	if (!RowNameString.StartsWith(TEXT("Item_")))
	{
		return false;
	}

	const ULSDropSettings* Settings = GetDefault<ULSDropSettings>();
	if (!Settings)
	{
		return false;
	}

	UDataTable* ItemTable = Settings->ItemTable.LoadSynchronous();
	const FLSItemRow* Row = ItemTable ? ItemTable->FindRow<FLSItemRow>(ItemRowName, TEXT("LSLobbyStorageFilter")) : nullptr;
	if (!Row)
	{
		return false;
	}

	return Row->Item_Type >= 4 && Row->Item_Type <= 9;
}

ULSSaveSubsystem* ULSLobbyStorageWidget::GetSaveSubsystem() const
{
	UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
}

bool ULSLobbyStorageWidget::IsFilledStorageSlot(const FLSSessionItem& Item)
{
	return !Item.ItemRowName.IsNone() && Item.Amount > 0;
}

#undef LOCTEXT_NAMESPACE
