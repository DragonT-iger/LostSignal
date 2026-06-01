#include "Inventory/LSInventorySlotUtils.h"

#include "Data/LSArmorRow.h"
#include "Data/LSChipRow.h"
#include "Data/LSDropSettings.h"
#include "Data/LSItemRow.h"
#include "Data/LSWeaponRow.h"
#include "Engine/DataTable.h"
#include "LostSignal.h"

namespace
{
// 알려진 등급 토큰. Row Name 파싱과 정렬 순서의 기준.
const TArray<FString>& GetKnownGrades()
{
	static const TArray<FString> KnownGrades = {
		TEXT("Supply"), TEXT("Standard"), TEXT("Precision"),
		TEXT("Tuning"), TEXT("Prototype"), TEXT("Masterpiece"),
	};
	return KnownGrades;
}

constexpr int32 SortGroupWeaponOffset = 100000;
constexpr int32 SortGroupArmorOffset = 200000;
constexpr int32 SortGroupItemOffset = 300000;

int32 FindRowOrder(UDataTable* Table, const FName RowName)
{
	if (!Table)
	{
		return MAX_int32 / 2;
	}

	const TArray<FName> RowNames = Table->GetRowNames();
	const int32 RowIndex = RowNames.IndexOfByKey(RowName);
	return RowIndex == INDEX_NONE ? MAX_int32 / 2 : RowIndex;
}

int32 ResolveItemSortKey(const FName ItemRowName)
{
	const ULSDropSettings* Settings = GetDefault<ULSDropSettings>();
	if (!Settings || ItemRowName.IsNone())
	{
		return MAX_int32;
	}

	const FString RowNameString = ItemRowName.ToString();
	if (RowNameString.StartsWith(TEXT("Chip_")))
	{
		return FindRowOrder(Settings->ChipTable.LoadSynchronous(), ItemRowName);
	}

	if (RowNameString.StartsWith(TEXT("Weapon_")))
	{
		return SortGroupWeaponOffset + FindRowOrder(Settings->WeaponTable.LoadSynchronous(), ItemRowName);
	}

	if (RowNameString.StartsWith(TEXT("Armor_")))
	{
		return SortGroupArmorOffset + FindRowOrder(Settings->ArmorTable.LoadSynchronous(), ItemRowName);
	}

	if (RowNameString.StartsWith(TEXT("Item_")))
	{
		return SortGroupItemOffset + FindRowOrder(Settings->ItemTable.LoadSynchronous(), ItemRowName);
	}

	return MAX_int32;
}

template<typename RowType>
int32 ResolveMaxStackFromTable(TSoftObjectPtr<UDataTable> TablePtr, const FName ItemRowName, const TCHAR* Context, const TCHAR* RowTypeName)
{
	UDataTable* Table = TablePtr.LoadSynchronous();
	const RowType* Row = Table ? Table->FindRow<RowType>(ItemRowName, Context) : nullptr;
	if (!Row)
	{
		UE_LOG(LogLS, Warning, TEXT("[Inventory] %s row missing for max stack: %s"), RowTypeName, *ItemRowName.ToString());
		return 1;
	}

	return Row->Item_Max;
}
}

namespace LSInventorySlotUtils
{
bool IsFilled(const FLSSessionItem& Item)
{
	return !Item.ItemRowName.IsNone() && Item.Amount > 0;
}

bool IsFilled(const FLSDropResult& Item)
{
	return !Item.ItemRowName.IsNone() && Item.Amount > 0;
}

FLSSessionItem MakeEmptyItem()
{
	return FLSSessionItem();
}

FLSSessionItem ToSessionItem(const FLSDropResult& Item)
{
	FLSSessionItem Result;
	Result.ItemRowName = Item.ItemRowName;
	Result.Amount = Item.Amount;
	Result.StatSeed = Item.StatSeed;
	return Result;
}

void SetDropResultFromSessionItem(FLSDropResult& TargetSlot, const FLSSessionItem& SourceItem)
{
	TargetSlot.ItemRowName = SourceItem.ItemRowName;
	TargetSlot.Amount = SourceItem.Amount;
	TargetSlot.ItemText = FText::GetEmpty();
	TargetSlot.StatSeed = SourceItem.StatSeed;
}

void ClearDropResult(FLSDropResult& TargetSlot)
{
	TargetSlot.ItemRowName = NAME_None;
	TargetSlot.Amount = 0;
	TargetSlot.ItemText = FText::GetEmpty();
}

int32 ResolveItemMaxStack(const FName ItemRowName, const TCHAR* Context)
{
	if (ItemRowName.IsNone())
	{
		UE_LOG(LogLS, Warning, TEXT("[Inventory] Cannot resolve max stack because ItemRowName is none."));
		return 1;
	}

	const ULSDropSettings* Settings = GetDefault<ULSDropSettings>();
	if (!Settings)
	{
		UE_LOG(LogLS, Warning, TEXT("[Inventory] Cannot resolve max stack because LS Drop Settings is missing."));
		return 1;
	}

	const FString RowNameString = ItemRowName.ToString();
	int32 MaxStack = 1;
	if (RowNameString.StartsWith(TEXT("Chip_")))
	{
		MaxStack = ResolveMaxStackFromTable<FLSChipRow>(Settings->ChipTable, ItemRowName, Context, TEXT("Chip"));
	}
	else if (RowNameString.StartsWith(TEXT("Weapon_")))
	{
		MaxStack = ResolveMaxStackFromTable<FLSWeaponRow>(Settings->WeaponTable, ItemRowName, Context, TEXT("Weapon"));
	}
	else if (RowNameString.StartsWith(TEXT("Armor_")))
	{
		MaxStack = ResolveMaxStackFromTable<FLSArmorRow>(Settings->ArmorTable, ItemRowName, Context, TEXT("Armor"));
	}
	else if (RowNameString.StartsWith(TEXT("Item_")))
	{
		MaxStack = ResolveMaxStackFromTable<FLSItemRow>(Settings->ItemTable, ItemRowName, Context, TEXT("Item"));
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("[Inventory] Unknown item row prefix for max stack: %s"), *ItemRowName.ToString());
	}

	if (MaxStack <= 0)
	{
		UE_LOG(LogLS, Warning, TEXT("[Inventory] Invalid Item_Max for %s: %d. Falling back to 1."), *ItemRowName.ToString(), MaxStack);
		return 1;
	}

	return MaxStack;
}

FString ResolveItemGradeFromRowName(const FName ItemRowName)
{
	if (ItemRowName.IsNone())
	{
		return FString();
	}

	TArray<FString> Tokens;
	ItemRowName.ToString().ParseIntoArray(Tokens, TEXT("_"), /*InCullEmpty=*/true);

	const TArray<FString>& KnownGrades = GetKnownGrades();
	for (const FString& Token : Tokens)
	{
		if (KnownGrades.Contains(Token))
		{
			return Token;
		}
	}

	UE_LOG(LogLS, Warning, TEXT("[Inventory] Cannot resolve grade from row name '%s' (no known grade token)."), *ItemRowName.ToString());
	return FString();
}

void EnsureSlotIndex(TArray<FLSSessionItem>& Slots, const int32 SlotIndex)
{
	while (Slots.Num() <= SlotIndex)
	{
		Slots.Add(MakeEmptyItem());
	}
}

bool TryAddItemsToSlotArray(TArray<FLSSessionItem>& Slots, const FName ItemRowName, int32 Amount, const int32 MaxSlotCount, const int32 StatSeed, FLSSessionItem& OutRemainingItem)
{
	OutRemainingItem = MakeEmptyItem();
	if (ItemRowName.IsNone() || Amount <= 0)
	{
		UE_LOG(LogLS, Warning, TEXT("[Inventory] Cannot add invalid item. Row=%s Amount=%d"), *ItemRowName.ToString(), Amount);
		return false;
	}

	if (MaxSlotCount <= 0)
	{
		OutRemainingItem.ItemRowName = ItemRowName;
		OutRemainingItem.Amount = Amount;
		OutRemainingItem.StatSeed = StatSeed;
		return false;
	}

	const int32 OriginalAmount = Amount;
	const int32 MaxStack = ResolveItemMaxStack(ItemRowName, TEXT("TryAddItemsToSlotArray"));
	const int32 ExistingSlotLimit = FMath::Min(Slots.Num(), MaxSlotCount);

	for (int32 SlotIndex = 0; SlotIndex < ExistingSlotLimit; ++SlotIndex)
	{
		FLSSessionItem& Slot = Slots[SlotIndex];
		if (Amount <= 0)
		{
			break;
		}

		// 인스턴스 시드가 다르면 같은 RowName이라도 합치지 않는다. (칩 등 개체별 스탯 보존)
		if (Slot.ItemRowName != ItemRowName || Slot.StatSeed != StatSeed || Slot.Amount >= MaxStack)
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

		if (IsFilled(Slot))
		{
			continue;
		}

		Slot.ItemRowName = ItemRowName;
		Slot.Amount = FMath::Min(Amount, MaxStack);
		Slot.StatSeed = StatSeed;
		Amount -= Slot.Amount;
	}

	while (Amount > 0 && Slots.Num() < MaxSlotCount)
	{
		FLSSessionItem NewSlot;
		NewSlot.ItemRowName = ItemRowName;
		NewSlot.Amount = FMath::Min(Amount, MaxStack);
		NewSlot.StatSeed = StatSeed;
		Slots.Add(NewSlot);
		Amount -= NewSlot.Amount;
	}

	if (Amount > 0)
	{
		OutRemainingItem.ItemRowName = ItemRowName;
		OutRemainingItem.Amount = Amount;
		OutRemainingItem.StatSeed = StatSeed;
	}

	return Amount < OriginalAmount;
}

void AddItemsToSlotArray(TArray<FLSSessionItem>& Slots, const FName ItemRowName, int32 Amount)
{
	if (ItemRowName.IsNone() || Amount <= 0)
	{
		return;
	}

	FLSSessionItem RemainingItem;
	while (Amount > 0)
	{
		const int32 OriginalAmount = Amount;
		TryAddItemsToSlotArray(Slots, ItemRowName, Amount, MAX_int32, /*StatSeed=*/0, RemainingItem);
		Amount = RemainingItem.Amount;
		if (Amount == OriginalAmount)
		{
			return;
		}
	}
}

void NormalizeSlotArray(TArray<FLSSessionItem>& Slots)
{
	TArray<FLSSessionItem> OldSlots = MoveTemp(Slots);
	Slots.Reset();

	for (const FLSSessionItem& OldSlot : OldSlots)
	{
		if (!IsFilled(OldSlot))
		{
			Slots.Add(MakeEmptyItem());
			continue;
		}

		const int32 MaxStack = ResolveItemMaxStack(OldSlot.ItemRowName, TEXT("NormalizeSlotArray"));
		FLSSessionItem NormalizedSlot;
		NormalizedSlot.ItemRowName = OldSlot.ItemRowName;
		NormalizedSlot.Amount = FMath::Min(OldSlot.Amount, MaxStack);
		Slots.Add(NormalizedSlot);

		const int32 OverflowAmount = OldSlot.Amount - NormalizedSlot.Amount;
		if (OverflowAmount > 0)
		{
			AddItemsToSlotArray(Slots, OldSlot.ItemRowName, OverflowAmount);
		}
	}
}

void RemoveItemsFromSlotArray(TArray<FLSSessionItem>& Slots, const FName ItemRowName, int32 Amount)
{
	if (ItemRowName.IsNone() || Amount <= 0)
	{
		return;
	}

	for (FLSSessionItem& Slot : Slots)
	{
		if (Amount <= 0)
		{
			break;
		}

		if (Slot.ItemRowName != ItemRowName || Slot.Amount <= 0)
		{
			continue;
		}

		const int32 RemoveAmount = FMath::Min(Amount, Slot.Amount);
		Slot.Amount -= RemoveAmount;
		Amount -= RemoveAmount;
	}

	for (FLSSessionItem& Slot : Slots)
	{
		if (Slot.Amount <= 0)
		{
			Slot = MakeEmptyItem();
		}
	}
}

void SortAndCompactSlotArray(TArray<FLSSessionItem>& Slots)
{
	const int32 OriginalSlotCount = Slots.Num();
	TMap<FName, int32> AmountByRowName;
	for (const FLSSessionItem& Slot : Slots)
	{
		if (IsFilled(Slot))
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
		const int32 LeftSortKey = ResolveItemSortKey(Left.ItemRowName);
		const int32 RightSortKey = ResolveItemSortKey(Right.ItemRowName);
		return LeftSortKey != RightSortKey ? LeftSortKey < RightSortKey : Left.ItemRowName.LexicalLess(Right.ItemRowName);
	});

	Slots.Reset();
	for (const FLSSessionItem& MergedItem : MergedItems)
	{
		AddItemsToSlotArray(Slots, MergedItem.ItemRowName, MergedItem.Amount);
	}

	while (Slots.Num() < OriginalSlotCount)
	{
		Slots.Add(MakeEmptyItem());
	}
}

bool SwapSlots(TArray<FLSSessionItem>& FromSlots, const int32 FromIndex, TArray<FLSSessionItem>& ToSlots, const int32 ToIndex, const int32 ToMaxSlotCount)
{
	if (!FromSlots.IsValidIndex(FromIndex) || !IsFilled(FromSlots[FromIndex]) || ToIndex < 0)
	{
		return false;
	}

	if (&FromSlots == &ToSlots && FromIndex == ToIndex)
	{
		return true;
	}

	if (ToMaxSlotCount != INDEX_NONE && ToIndex >= ToMaxSlotCount)
	{
		return false;
	}

	EnsureSlotIndex(ToSlots, ToIndex);
	Swap(FromSlots[FromIndex], ToSlots[ToIndex]);
	return true;
}

bool MoveSlotWithinArray(TArray<FLSSessionItem>& Slots, const int32 FromIndex, const int32 ToIndex)
{
	if (!Slots.IsValidIndex(FromIndex) || !IsFilled(Slots[FromIndex]) || ToIndex < 0)
	{
		return false;
	}

	if (FromIndex == ToIndex)
	{
		return true;
	}

	FLSSessionItem MovingItem = Slots[FromIndex];
	Slots.RemoveAt(FromIndex);

	const int32 ClampedTargetIndex = FMath::Clamp(ToIndex, 0, Slots.Num());
	Slots.Insert(MovingItem, ClampedTargetIndex);
	return true;
}

bool DropSlot(TArray<FLSSessionItem>& FromSlots, const int32 FromIndex, TArray<FLSSessionItem>& ToSlots, const int32 ToIndex, const int32 ToMaxSlotCount)
{
	if (!FromSlots.IsValidIndex(FromIndex) || !IsFilled(FromSlots[FromIndex]) || ToIndex < 0)
	{
		return false;
	}

	if (&FromSlots == &ToSlots && FromIndex == ToIndex)
	{
		return true;
	}

	if (ToMaxSlotCount != INDEX_NONE && ToIndex >= ToMaxSlotCount)
	{
		return false;
	}

	EnsureSlotIndex(ToSlots, ToIndex);
	FLSSessionItem& FromSlot = FromSlots[FromIndex];
	FLSSessionItem& ToSlot = ToSlots[ToIndex];

	if (!IsFilled(ToSlot))
	{
		ToSlot = FromSlot;
		FromSlot = MakeEmptyItem();
		return true;
	}

	if (FromSlot.ItemRowName == ToSlot.ItemRowName)
	{
		const int32 MaxStack = ResolveItemMaxStack(FromSlot.ItemRowName, TEXT("DropSlot"));
		const int32 AddAmount = FMath::Min(FromSlot.Amount, MaxStack - ToSlot.Amount);
		if (AddAmount <= 0)
		{
			return false;
		}

		ToSlot.Amount += AddAmount;
		FromSlot.Amount -= AddAmount;
		if (FromSlot.Amount <= 0)
		{
			FromSlot = MakeEmptyItem();
		}
		return true;
	}

	Swap(FromSlot, ToSlot);
	return true;
}

bool DropExternalItemToSlot(FLSSessionItem& InOutExternalItem, TArray<FLSSessionItem>& ToSlots, const int32 ToIndex, const int32 ToMaxSlotCount)
{
	if (!IsFilled(InOutExternalItem) || ToIndex < 0)
	{
		return false;
	}

	if (ToMaxSlotCount != INDEX_NONE && ToIndex >= ToMaxSlotCount)
	{
		return false;
	}

	EnsureSlotIndex(ToSlots, ToIndex);
	FLSSessionItem& ToSlot = ToSlots[ToIndex];
	if (!IsFilled(ToSlot))
	{
		ToSlot = InOutExternalItem;
		InOutExternalItem = MakeEmptyItem();
		return true;
	}

	if (InOutExternalItem.ItemRowName == ToSlot.ItemRowName)
	{
		const int32 MaxStack = ResolveItemMaxStack(InOutExternalItem.ItemRowName, TEXT("DropExternalItemToSlot"));
		const int32 AddAmount = FMath::Min(InOutExternalItem.Amount, MaxStack - ToSlot.Amount);
		if (AddAmount <= 0)
		{
			return false;
		}

		ToSlot.Amount += AddAmount;
		InOutExternalItem.Amount -= AddAmount;
		if (InOutExternalItem.Amount <= 0)
		{
			InOutExternalItem = MakeEmptyItem();
		}
		return true;
	}

	Swap(InOutExternalItem, ToSlot);
	return true;
}

bool DropResultSlot(TArray<FLSDropResult>& Slots, const int32 FromIndex, const int32 ToIndex)
{
	if (!Slots.IsValidIndex(FromIndex) || !IsFilled(Slots[FromIndex]) || !Slots.IsValidIndex(ToIndex))
	{
		return false;
	}

	if (FromIndex == ToIndex)
	{
		return true;
	}

	FLSDropResult& FromSlot = Slots[FromIndex];
	FLSDropResult& ToSlot = Slots[ToIndex];
	if (!IsFilled(ToSlot))
	{
		ToSlot = FromSlot;
		ClearDropResult(FromSlot);
		return true;
	}

	if (FromSlot.ItemRowName == ToSlot.ItemRowName)
	{
		const int32 MaxStack = ResolveItemMaxStack(FromSlot.ItemRowName, TEXT("DropResultSlot"));
		const int32 AddAmount = FMath::Min(FromSlot.Amount, MaxStack - ToSlot.Amount);
		if (AddAmount <= 0)
		{
			return false;
		}

		ToSlot.Amount += AddAmount;
		FromSlot.Amount -= AddAmount;
		ToSlot.ItemText = FText::GetEmpty();
		if (FromSlot.Amount <= 0)
		{
			ClearDropResult(FromSlot);
		}
		else
		{
			FromSlot.ItemText = FText::GetEmpty();
		}
		return true;
	}

	Swap(FromSlot, ToSlot);
	return true;
}
}
