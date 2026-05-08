#include "UI/Inventory/LSInventoryWidget.h"

#include "LostSignal.h"
#include "Components/WrapBox.h"
#include "Data/LSArmorRow.h"
#include "Data/LSChipRow.h"
#include "Data/LSDropSettings.h"
#include "Data/LSItemRow.h"
#include "Data/LSWeaponRow.h"
#include "Engine/DataTable.h"
#include "Session/LSSaveSubsystem.h"
#include "Session/LSSessionSubsystem.h"
#include "UI/Inventory/LSInventoryItemSlotWidget.h"

namespace
{
void AppendSlotItems(TArray<FLSSessionItem>& Items, const TArray<FLSSessionItem>& NewItems)
{
	for (const FLSSessionItem& NewItem : NewItems)
	{
		if (NewItem.ItemRowName.IsNone() || NewItem.Amount <= 0)
		{
			UE_LOG(LogLS, Warning, TEXT("Skipping invalid inventory slot item. Row=%s Amount=%d"), *NewItem.ItemRowName.ToString(), NewItem.Amount);
			continue;
		}

		Items.Add(NewItem);
	}
}

int32 ResolveItemMaxStackForInventoryUI(const FName ItemRowName)
{
	const ULSDropSettings* Settings = GetDefault<ULSDropSettings>();
	if (!Settings || ItemRowName.IsNone())
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot resolve UI max stack. Row=%s"), *ItemRowName.ToString());
		return 1;
	}

	const FString RowNameString = ItemRowName.ToString();
	int32 MaxStack = 1;

	if (RowNameString.StartsWith(TEXT("Chip_")))
	{
		UDataTable* Table = Settings->ChipTable.LoadSynchronous();
		const FLSChipRow* Row = Table ? Table->FindRow<FLSChipRow>(ItemRowName, TEXT("ResolveItemMaxStackForInventoryUI")) : nullptr;
		MaxStack = Row ? Row->Item_Max : 1;
		if (!Row) UE_LOG(LogLS, Warning, TEXT("Cannot resolve UI max stack because chip row is missing: %s"), *ItemRowName.ToString());
	}
	else if (RowNameString.StartsWith(TEXT("Weapon_")))
	{
		UDataTable* Table = Settings->WeaponTable.LoadSynchronous();
		const FLSWeaponRow* Row = Table ? Table->FindRow<FLSWeaponRow>(ItemRowName, TEXT("ResolveItemMaxStackForInventoryUI")) : nullptr;
		MaxStack = Row ? Row->Item_Max : 1;
		if (!Row) UE_LOG(LogLS, Warning, TEXT("Cannot resolve UI max stack because weapon row is missing: %s"), *ItemRowName.ToString());
	}
	else if (RowNameString.StartsWith(TEXT("Armor_")))
	{
		UDataTable* Table = Settings->ArmorTable.LoadSynchronous();
		const FLSArmorRow* Row = Table ? Table->FindRow<FLSArmorRow>(ItemRowName, TEXT("ResolveItemMaxStackForInventoryUI")) : nullptr;
		MaxStack = Row ? Row->Item_Max : 1;
		if (!Row) UE_LOG(LogLS, Warning, TEXT("Cannot resolve UI max stack because armor row is missing: %s"), *ItemRowName.ToString());
	}
	else if (RowNameString.StartsWith(TEXT("Item_")))
	{
		UDataTable* Table = Settings->ItemTable.LoadSynchronous();
		const FLSItemRow* Row = Table ? Table->FindRow<FLSItemRow>(ItemRowName, TEXT("ResolveItemMaxStackForInventoryUI")) : nullptr;
		MaxStack = Row ? Row->Item_Max : 1;
		if (!Row) UE_LOG(LogLS, Warning, TEXT("Cannot resolve UI max stack because item row is missing: %s"), *ItemRowName.ToString());
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot resolve UI max stack because row has unknown prefix: %s"), *ItemRowName.ToString());
	}

	if (MaxStack <= 0)
	{
		UE_LOG(LogLS, Warning, TEXT("Invalid UI Item_Max for %s: %d. Falling back to 1."), *ItemRowName.ToString(), MaxStack);
		return 1;
	}

	return MaxStack;
}

void AddSlotItemWithStackRules(TArray<FLSSessionItem>& Slots, const FLSSessionItem& NewItem)
{
	if (NewItem.ItemRowName.IsNone() || NewItem.Amount <= 0)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot add invalid UI slot item. Row=%s Amount=%d"), *NewItem.ItemRowName.ToString(), NewItem.Amount);
		return;
	}

	const int32 MaxStack = ResolveItemMaxStackForInventoryUI(NewItem.ItemRowName);
	int32 RemainingAmount = NewItem.Amount;

	for (FLSSessionItem& Slot : Slots)
	{
		if (RemainingAmount <= 0)
		{
			return;
		}

		if (Slot.ItemRowName != NewItem.ItemRowName || Slot.Amount >= MaxStack)
		{
			continue;
		}

		const int32 AddAmount = FMath::Min(RemainingAmount, MaxStack - Slot.Amount);
		Slot.Amount += AddAmount;
		RemainingAmount -= AddAmount;
	}

	while (RemainingAmount > 0)
	{
		FLSSessionItem NewSlot;
		NewSlot.ItemRowName = NewItem.ItemRowName;
		NewSlot.Amount = FMath::Min(RemainingAmount, MaxStack);
		Slots.Add(NewSlot);
		RemainingAmount -= NewSlot.Amount;
	}
}

void AddSlotItemsWithStackRules(TArray<FLSSessionItem>& Slots, const TArray<FLSSessionItem>& NewItems)
{
	for (const FLSSessionItem& NewItem : NewItems)
	{
		AddSlotItemWithStackRules(Slots, NewItem);
	}
}
}

void ULSInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RebuildInventorySlots();
	RebuildConfirmedStorageSlots();
}

void ULSInventoryWidget::SetInventorySlotCount(const int32 NewInventorySlotCount)
{
	InventorySlotCount = FMath::Max(0, NewInventorySlotCount);
	RebuildInventorySlots();
}

void ULSInventoryWidget::SetConfirmedStorageSlotCount(const int32 NewConfirmedStorageSlotCount)
{
	ConfirmedStorageSlotCount = FMath::Max(0, NewConfirmedStorageSlotCount);
	RebuildConfirmedStorageSlots();
}

void ULSInventoryWidget::RebuildInventorySlots()
{
	if (!InventoryWrapBox)
	{
		UE_LOG(LogLS, Warning, TEXT("InventoryWrapBox is not bound on %s."), *GetNameSafe(this));
		return;
	}

	InventoryWrapBox->ClearChildren();

	if (!InventoryItemSlotWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("InventoryItemSlotWidgetClass is not set on %s."), *GetNameSafe(this));
		return;
	}

	APlayerController* OwningPlayer = GetOwningPlayer();
	UWorld* World = GetWorld();
	if (!OwningPlayer && !World)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot rebuild inventory slots because owner/world is missing on %s."), *GetNameSafe(this));
		return;
	}

	TArray<FLSSessionItem> InventoryItems;
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (ULSSaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<ULSSaveSubsystem>())
		{
			AppendSlotItems(InventoryItems, SaveSubsystem->GetStash());
		}
		else
		{
			UE_LOG(LogLS, Warning, TEXT("SaveSubsystem is missing on %s."), *GetNameSafe(this));
		}

		if (ULSSessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<ULSSessionSubsystem>())
		{
			AddSlotItemsWithStackRules(InventoryItems, SessionSubsystem->GetSessionInventory());
		}
		else
		{
			UE_LOG(LogLS, Warning, TEXT("SessionSubsystem is missing on %s."), *GetNameSafe(this));
		}

		UE_LOG(LogLS, Log, TEXT("InventoryWidget rebuilt with %d slot items on %s."), InventoryItems.Num(), *GetNameSafe(this));
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("GameInstance is missing on %s."), *GetNameSafe(this));
	}

	const int32 SlotCountToBuild = FMath::Max(InventorySlotCount, InventoryItems.Num());
	for (int32 SlotIndex = 0; SlotIndex < SlotCountToBuild; ++SlotIndex)
	{
		ULSInventoryItemSlotWidget* SlotWidget = OwningPlayer
			? CreateWidget<ULSInventoryItemSlotWidget>(OwningPlayer, InventoryItemSlotWidgetClass)
			: CreateWidget<ULSInventoryItemSlotWidget>(World, InventoryItemSlotWidgetClass);

		if (SlotWidget)
		{
			if (InventoryItems.IsValidIndex(SlotIndex))
			{
				SlotWidget->SetItem(InventoryItems[SlotIndex].ItemRowName, InventoryItems[SlotIndex].Amount);
			}
			else
			{
				SlotWidget->ClearItem();
			}

			InventoryWrapBox->AddChildToWrapBox(SlotWidget);
		}
		else
		{
			UE_LOG(LogLS, Warning, TEXT("Failed to create inventory slot widget at index %d on %s."), SlotIndex, *GetNameSafe(this));
		}
	}
}

void ULSInventoryWidget::RebuildConfirmedStorageSlots()
{
	if (!ConfirmedStorageSlotWrapBox)
	{
		UE_LOG(LogLS, Warning, TEXT("ConfirmedStorageSlotWrapBox is not bound on %s."), *GetNameSafe(this));
		return;
	}

	ConfirmedStorageSlotWrapBox->ClearChildren();

	if (!InventoryItemSlotWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("InventoryItemSlotWidgetClass is not set on %s. Confirmed storage slots use the same widget class."), *GetNameSafe(this));
		return;
	}

	APlayerController* OwningPlayer = GetOwningPlayer();
	UWorld* World = GetWorld();
	if (!OwningPlayer && !World)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot rebuild confirmed storage slots because owner/world is missing on %s."), *GetNameSafe(this));
		return;
	}

	for (int32 SlotIndex = 0; SlotIndex < ConfirmedStorageSlotCount; ++SlotIndex)
	{
		ULSInventoryItemSlotWidget* SlotWidget = OwningPlayer
			? CreateWidget<ULSInventoryItemSlotWidget>(OwningPlayer, InventoryItemSlotWidgetClass)
			: CreateWidget<ULSInventoryItemSlotWidget>(World, InventoryItemSlotWidgetClass);

		if (SlotWidget)
		{
			SlotWidget->ClearItem();
			ConfirmedStorageSlotWrapBox->AddChildToWrapBox(SlotWidget);
		}
		else
		{
			UE_LOG(LogLS, Warning, TEXT("Failed to create confirmed storage slot widget at index %d on %s."), SlotIndex, *GetNameSafe(this));
		}
	}
}
