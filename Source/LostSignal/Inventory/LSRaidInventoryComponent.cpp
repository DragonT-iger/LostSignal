#include "Inventory/LSRaidInventoryComponent.h"

#include "Data/LSArmorRow.h"
#include "Data/LSChipRow.h"
#include "Data/LSDropSettings.h"
#include "Data/LSItemRow.h"
#include "Data/LSWeaponRow.h"
#include "Engine/DataTable.h"
#include "LostSignal.h"

namespace
{
constexpr int32 DefaultMaxInventorySlotCount = 10;

bool IsFilledRaidInventorySlot(const FLSSessionItem& Item)
{
	return !Item.ItemRowName.IsNone() && Item.Amount > 0;
}

FLSSessionItem MakeEmptyRaidInventorySlot()
{
	return FLSSessionItem();
}

int32 ResolveItemMaxStackForRaidInventory(const FName ItemRowName)
{
	const ULSDropSettings* Settings = GetDefault<ULSDropSettings>();
	if (!Settings || ItemRowName.IsNone())
	{
		UE_LOG(LogLS, Warning, TEXT("[RaidInventory] Cannot resolve max stack. Row=%s"), *ItemRowName.ToString());
		return 1;
	}

	const FString RowNameString = ItemRowName.ToString();
	int32 MaxStack = 1;

	if (RowNameString.StartsWith(TEXT("Chip_")))
	{
		UDataTable* Table = Settings->ChipTable.LoadSynchronous();
		const FLSChipRow* Row = Table ? Table->FindRow<FLSChipRow>(ItemRowName, TEXT("ResolveItemMaxStackForRaidInventory")) : nullptr;
		MaxStack = Row ? Row->Item_Max : 1;
		if (!Row) UE_LOG(LogLS, Warning, TEXT("[RaidInventory] Chip row missing for max stack: %s"), *ItemRowName.ToString());
	}
	else if (RowNameString.StartsWith(TEXT("Weapon_")))
	{
		UDataTable* Table = Settings->WeaponTable.LoadSynchronous();
		const FLSWeaponRow* Row = Table ? Table->FindRow<FLSWeaponRow>(ItemRowName, TEXT("ResolveItemMaxStackForRaidInventory")) : nullptr;
		MaxStack = Row ? Row->Item_Max : 1;
		if (!Row) UE_LOG(LogLS, Warning, TEXT("[RaidInventory] Weapon row missing for max stack: %s"), *ItemRowName.ToString());
	}
	else if (RowNameString.StartsWith(TEXT("Armor_")))
	{
		UDataTable* Table = Settings->ArmorTable.LoadSynchronous();
		const FLSArmorRow* Row = Table ? Table->FindRow<FLSArmorRow>(ItemRowName, TEXT("ResolveItemMaxStackForRaidInventory")) : nullptr;
		MaxStack = Row ? Row->Item_Max : 1;
		if (!Row) UE_LOG(LogLS, Warning, TEXT("[RaidInventory] Armor row missing for max stack: %s"), *ItemRowName.ToString());
	}
	else if (RowNameString.StartsWith(TEXT("Item_")))
	{
		UDataTable* Table = Settings->ItemTable.LoadSynchronous();
		const FLSItemRow* Row = Table ? Table->FindRow<FLSItemRow>(ItemRowName, TEXT("ResolveItemMaxStackForRaidInventory")) : nullptr;
		MaxStack = Row ? Row->Item_Max : 1;
		if (!Row) UE_LOG(LogLS, Warning, TEXT("[RaidInventory] Item row missing for max stack: %s"), *ItemRowName.ToString());
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("[RaidInventory] Unknown item row prefix for max stack: %s"), *ItemRowName.ToString());
	}

	if (MaxStack <= 0)
	{
		UE_LOG(LogLS, Warning, TEXT("[RaidInventory] Invalid Item_Max for %s: %d. Falling back to 1."), *ItemRowName.ToString(), MaxStack);
		return 1;
	}

	return MaxStack;
}

bool TryAddItemsToRaidInventorySlotArray(TArray<FLSSessionItem>& Slots, const FName ItemRowName, int32 Amount, const int32 MaxSlotCount, FLSSessionItem& OutRemainingItem)
{
	OutRemainingItem = FLSSessionItem();
	if (ItemRowName.IsNone() || Amount <= 0)
	{
		UE_LOG(LogLS, Warning, TEXT("[RaidInventory] Cannot add invalid item. Row=%s Amount=%d"), *ItemRowName.ToString(), Amount);
		return false;
	}

	if (MaxSlotCount <= 0)
	{
		OutRemainingItem.ItemRowName = ItemRowName;
		OutRemainingItem.Amount = Amount;
		return false;
	}

	const int32 OriginalAmount = Amount;
	const int32 MaxStack = ResolveItemMaxStackForRaidInventory(ItemRowName);
	const int32 ExistingSlotLimit = FMath::Min(Slots.Num(), MaxSlotCount);
	for (int32 SlotIndex = 0; SlotIndex < ExistingSlotLimit; ++SlotIndex)
	{
		FLSSessionItem& Slot = Slots[SlotIndex];
		if (Amount <= 0)
		{
			break;
		}

		if (Slot.ItemRowName != ItemRowName || Slot.Amount >= MaxStack)
		{
			continue;
		}

		const int32 AddAmount = FMath::Min(Amount, MaxStack - Slot.Amount);
		Slot.Amount += AddAmount;
		Amount -= AddAmount;
	}

	for (int32 SlotIndex = 0; SlotIndex < ExistingSlotLimit; ++SlotIndex)
	{
		FLSSessionItem& Slot = Slots[SlotIndex];
		if (Amount <= 0)
		{
			break;
		}

		if (IsFilledRaidInventorySlot(Slot))
		{
			continue;
		}

		Slot.ItemRowName = ItemRowName;
		Slot.Amount = FMath::Min(Amount, MaxStack);
		Amount -= Slot.Amount;
	}

	while (Amount > 0 && Slots.Num() < MaxSlotCount)
	{
		FLSSessionItem NewSlot;
		NewSlot.ItemRowName = ItemRowName;
		NewSlot.Amount = FMath::Min(Amount, MaxStack);
		Slots.Add(NewSlot);
		Amount -= NewSlot.Amount;
	}

	if (Amount > 0)
	{
		OutRemainingItem.ItemRowName = ItemRowName;
		OutRemainingItem.Amount = Amount;
	}

	return Amount < OriginalAmount;
}

void AddItemsToRaidInventorySlotArray(TArray<FLSSessionItem>& Slots, const FName ItemRowName, int32 Amount)
{
	if (ItemRowName.IsNone() || Amount <= 0)
	{
		return;
	}

	FLSSessionItem RemainingItem;
	while (Amount > 0)
	{
		const int32 OriginalAmount = Amount;
		TryAddItemsToRaidInventorySlotArray(Slots, ItemRowName, Amount, MAX_int32, RemainingItem);
		Amount = RemainingItem.Amount;
		if (Amount == OriginalAmount)
		{
			return;
		}
	}
}

int32 FindRaidInventoryRowOrder(UDataTable* Table, const FName RowName)
{
	if (!Table)
	{
		return MAX_int32 / 2;
	}

	const TArray<FName> RowNames = Table->GetRowNames();
	const int32 RowIndex = RowNames.IndexOfByKey(RowName);
	return RowIndex == INDEX_NONE ? MAX_int32 / 2 : RowIndex;
}

int32 ResolveItemSortKeyForRaidInventory(const FName ItemRowName)
{
	const ULSDropSettings* Settings = GetDefault<ULSDropSettings>();
	if (!Settings || ItemRowName.IsNone())
	{
		return MAX_int32;
	}

	const FString RowNameString = ItemRowName.ToString();
	if (RowNameString.StartsWith(TEXT("Chip_")))
	{
		return FindRaidInventoryRowOrder(Settings->ChipTable.LoadSynchronous(), ItemRowName);
	}

	if (RowNameString.StartsWith(TEXT("Weapon_")))
	{
		return 100000 + FindRaidInventoryRowOrder(Settings->WeaponTable.LoadSynchronous(), ItemRowName);
	}

	if (RowNameString.StartsWith(TEXT("Armor_")))
	{
		return 200000 + FindRaidInventoryRowOrder(Settings->ArmorTable.LoadSynchronous(), ItemRowName);
	}

	if (RowNameString.StartsWith(TEXT("Item_")))
	{
		return 300000 + FindRaidInventoryRowOrder(Settings->ItemTable.LoadSynchronous(), ItemRowName);
	}

	return MAX_int32;
}

void SortAndCompactRaidInventorySlotArray(TArray<FLSSessionItem>& Slots)
{
	const int32 OriginalSlotCount = Slots.Num();
	TMap<FName, int32> AmountByRowName;
	for (const FLSSessionItem& Slot : Slots)
	{
		if (IsFilledRaidInventorySlot(Slot))
		{
			AmountByRowName.FindOrAdd(Slot.ItemRowName) += Slot.Amount;
		}
	}

	TArray<FLSSessionItem> MergedItems;
	MergedItems.Reserve(AmountByRowName.Num());
	for (const TPair<FName, int32>& Pair : AmountByRowName)
	{
		MergedItems.Add({ Pair.Key, Pair.Value });
	}

	MergedItems.Sort([](const FLSSessionItem& Left, const FLSSessionItem& Right)
	{
		const int32 LeftSortKey = ResolveItemSortKeyForRaidInventory(Left.ItemRowName);
		const int32 RightSortKey = ResolveItemSortKeyForRaidInventory(Right.ItemRowName);
		return LeftSortKey != RightSortKey ? LeftSortKey < RightSortKey : Left.ItemRowName.LexicalLess(Right.ItemRowName);
	});

	Slots.Reset();
	for (const FLSSessionItem& MergedItem : MergedItems)
	{
		AddItemsToRaidInventorySlotArray(Slots, MergedItem.ItemRowName, MergedItem.Amount);
	}

	while (Slots.Num() < OriginalSlotCount)
	{
		Slots.Add(MakeEmptyRaidInventorySlot());
	}
}

void EnsureRaidInventorySlotIndex(TArray<FLSSessionItem>& Slots, const int32 SlotIndex)
{
	while (Slots.Num() <= SlotIndex)
	{
		Slots.Add(MakeEmptyRaidInventorySlot());
	}
}
}

ULSRaidInventoryComponent::ULSRaidInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULSRaidInventoryComponent::StartRaidInventory(const TArray<FLSSessionItem>& Loadout, const TArray<FLSSessionItem>& SafeItems)
{
	LoadoutSnapshot.Items = Loadout;
	SessionInventory = Loadout;
	SessionSafeInventory = SafeItems;
	ConsumedItems.Empty();
	bRaidActive = true;
}

void ULSRaidInventoryComponent::MirrorRaidInventoryState(const TArray<FLSSessionItem>& InventoryItems, const TArray<FLSSessionItem>& SafeItems)
{
	SessionInventory = InventoryItems;
	SessionSafeInventory = SafeItems;
	bRaidActive = true;
}

void ULSRaidInventoryComponent::EndRaidInventory()
{
	bRaidActive = false;
}

int32 ULSRaidInventoryComponent::GetMaxInventorySlotCount() const
{
	return DefaultMaxInventorySlotCount;
}

bool ULSRaidInventoryComponent::TryAddSessionItem(const FName ItemRowName, const int32 Amount, FLSSessionItem& OutRemainingItem)
{
	return TryAddItemsToRaidInventorySlotArray(SessionInventory, ItemRowName, Amount, GetMaxInventorySlotCount(), OutRemainingItem);
}

void ULSRaidInventoryComponent::SortSessionInventory()
{
	SortAndCompactRaidInventorySlotArray(SessionInventory);
}

bool ULSRaidInventoryComponent::DropSessionSlot(const ELSInventorySlotArea FromArea, const int32 FromIndex, const ELSInventorySlotArea ToArea, const int32 ToIndex)
{
	TArray<FLSSessionItem>* FromSlots = FromArea == ELSInventorySlotArea::Safe ? &SessionSafeInventory : &SessionInventory;
	TArray<FLSSessionItem>* ToSlots = ToArea == ELSInventorySlotArea::Safe ? &SessionSafeInventory : &SessionInventory;

	if (!FromSlots->IsValidIndex(FromIndex) || !IsFilledRaidInventorySlot((*FromSlots)[FromIndex]) || ToIndex < 0)
	{
		return false;
	}

	if (FromSlots == ToSlots && FromIndex == ToIndex)
	{
		return true;
	}

	if (ToArea == ELSInventorySlotArea::Inventory && ToIndex >= GetMaxInventorySlotCount())
	{
		return false;
	}

	EnsureRaidInventorySlotIndex(*ToSlots, ToIndex);
	FLSSessionItem& FromSlot = (*FromSlots)[FromIndex];
	FLSSessionItem& ToSlot = (*ToSlots)[ToIndex];

	if (!IsFilledRaidInventorySlot(ToSlot))
	{
		ToSlot = FromSlot;
		FromSlot = MakeEmptyRaidInventorySlot();
		return true;
	}

	if (FromSlot.ItemRowName == ToSlot.ItemRowName)
	{
		const int32 MaxStack = ResolveItemMaxStackForRaidInventory(FromSlot.ItemRowName);
		const int32 AddAmount = FMath::Min(FromSlot.Amount, MaxStack - ToSlot.Amount);
		if (AddAmount <= 0)
		{
			return false;
		}

		ToSlot.Amount += AddAmount;
		FromSlot.Amount -= AddAmount;
		if (FromSlot.Amount <= 0)
		{
			FromSlot = MakeEmptyRaidInventorySlot();
		}
		return true;
	}

	Swap(FromSlot, ToSlot);
	return true;
}

bool ULSRaidInventoryComponent::DropExternalItemToSessionSlot(FLSSessionItem& InOutExternalItem, const ELSInventorySlotArea ToArea, const int32 ToIndex)
{
	TArray<FLSSessionItem>* ToSlots = ToArea == ELSInventorySlotArea::Safe ? &SessionSafeInventory : &SessionInventory;
	if (!IsFilledRaidInventorySlot(InOutExternalItem) || ToIndex < 0)
	{
		return false;
	}

	if (ToArea == ELSInventorySlotArea::Inventory && ToIndex >= GetMaxInventorySlotCount())
	{
		return false;
	}

	EnsureRaidInventorySlotIndex(*ToSlots, ToIndex);
	FLSSessionItem& ToSlot = (*ToSlots)[ToIndex];
	if (!IsFilledRaidInventorySlot(ToSlot))
	{
		ToSlot = InOutExternalItem;
		InOutExternalItem = MakeEmptyRaidInventorySlot();
		return true;
	}

	if (InOutExternalItem.ItemRowName == ToSlot.ItemRowName)
	{
		const int32 MaxStack = ResolveItemMaxStackForRaidInventory(InOutExternalItem.ItemRowName);
		const int32 AddAmount = FMath::Min(InOutExternalItem.Amount, MaxStack - ToSlot.Amount);
		if (AddAmount <= 0)
		{
			return false;
		}

		ToSlot.Amount += AddAmount;
		InOutExternalItem.Amount -= AddAmount;
		if (InOutExternalItem.Amount <= 0)
		{
			InOutExternalItem = MakeEmptyRaidInventorySlot();
		}
		return true;
	}

	Swap(InOutExternalItem, ToSlot);
	return true;
}

bool ULSRaidInventoryComponent::GetSessionSlotItem(const ELSInventorySlotArea SlotArea, const int32 SlotIndex, FLSSessionItem& OutItem) const
{
	const TArray<FLSSessionItem>& Slots = SlotArea == ELSInventorySlotArea::Safe ? SessionSafeInventory : SessionInventory;
	if (!Slots.IsValidIndex(SlotIndex) || !IsFilledRaidInventorySlot(Slots[SlotIndex]))
	{
		return false;
	}

	OutItem = Slots[SlotIndex];
	return true;
}

bool ULSRaidInventoryComponent::ClearSessionSlot(const ELSInventorySlotArea SlotArea, const int32 SlotIndex)
{
	TArray<FLSSessionItem>& Slots = SlotArea == ELSInventorySlotArea::Safe ? SessionSafeInventory : SessionInventory;
	if (!Slots.IsValidIndex(SlotIndex) || !IsFilledRaidInventorySlot(Slots[SlotIndex]))
	{
		return false;
	}

	Slots[SlotIndex] = MakeEmptyRaidInventorySlot();
	return true;
}

bool ULSRaidInventoryComponent::ReplaceSessionSlotItem(const ELSInventorySlotArea SlotArea, const int32 SlotIndex, const FLSSessionItem& NewItem, FLSSessionItem& OutPreviousItem)
{
	OutPreviousItem = FLSSessionItem();
	if (SlotIndex < 0)
	{
		return false;
	}

	if (SlotArea == ELSInventorySlotArea::Inventory && SlotIndex >= GetMaxInventorySlotCount())
	{
		return false;
	}

	TArray<FLSSessionItem>& Slots = SlotArea == ELSInventorySlotArea::Safe ? SessionSafeInventory : SessionInventory;
	EnsureRaidInventorySlotIndex(Slots, SlotIndex);
	OutPreviousItem = Slots[SlotIndex];
	Slots[SlotIndex] = NewItem;
	return true;
}
