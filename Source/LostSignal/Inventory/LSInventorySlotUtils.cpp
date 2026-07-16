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

// 칩 기능 토큰(Row Name 접미사) → 기능별 아이콘 에셋명. 등급과 무관하게 기능별 4종으로 통합한다.
const TMap<FString, FString>& GetChipFunctionIconNames()
{
	static const TMap<FString, FString> ChipFunctionIconNames = {
		{ TEXT("HP"), TEXT("Living_chip") },
		{ TEXT("Inventory"), TEXT("Carrying_chip") },
		{ TEXT("Battle"), TEXT("Battle_chip") },
		{ TEXT("Navigation"), TEXT("Quest_chip") },
	};
	return ChipFunctionIconNames;
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
LSInventorySlotUtils::FLSItemTradeInfo ResolveTradeInfoFromTable(TSoftObjectPtr<UDataTable> TablePtr, const FName ItemRowName, const TCHAR* RowTypeName)
{
	LSInventorySlotUtils::FLSItemTradeInfo Info;
	UDataTable* Table = TablePtr.LoadSynchronous();
	const RowType* Row = Table ? Table->FindRow<RowType>(ItemRowName, TEXT("ResolveItemTradeInfo")) : nullptr;
	if (!Row)
	{
		UE_LOG(LogLS, Warning, TEXT("[Inventory] %s row missing for trade info: %s"), RowTypeName, *ItemRowName.ToString());
		return Info;
	}

	Info.Name = Row->Item_Text;
	Info.Description = Row->Item_Description;
	Info.Cost = Row->Item_Cost;
	Info.bValid = true;
	return Info;
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

int32 FindFirstEmptySlotIndex(const TArray<FLSSessionItem>& Slots, const int32 MaxSlotCount)
{
	for (int32 SlotIndex = 0; SlotIndex < MaxSlotCount; ++SlotIndex)
	{
		if (!Slots.IsValidIndex(SlotIndex) || !IsFilled(Slots[SlotIndex]))
		{
			return SlotIndex;
		}
	}
	return INDEX_NONE;
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
	Result.ChipStats = Item.ChipStats;
	return Result;
}

void SetDropResultFromSessionItem(FLSDropResult& TargetSlot, const FLSSessionItem& SourceItem)
{
	TargetSlot.ItemRowName = SourceItem.ItemRowName;
	TargetSlot.Amount = SourceItem.Amount;
	TargetSlot.ItemText = FText::GetEmpty();
	TargetSlot.ChipStats = SourceItem.ChipStats;
}

void ClearDropResult(FLSDropResult& TargetSlot)
{
	TargetSlot.ItemRowName = NAME_None;
	TargetSlot.Amount = 0;
	TargetSlot.ItemText = FText::GetEmpty();
	TargetSlot.ChipStats.Reset();
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

FLSItemTradeInfo ResolveItemTradeInfo(const FName ItemRowName)
{
	FLSItemTradeInfo Info;
	const ULSDropSettings* Settings = GetDefault<ULSDropSettings>();
	if (ItemRowName.IsNone() || !Settings)
	{
		UE_LOG(LogLS, Warning, TEXT("[Inventory] Cannot resolve trade info. Row=%s"), *ItemRowName.ToString());
		return Info;
	}

	const FString RowNameString = ItemRowName.ToString();
	if (RowNameString.StartsWith(TEXT("Chip_")))
	{
		return ResolveTradeInfoFromTable<FLSChipRow>(Settings->ChipTable, ItemRowName, TEXT("Chip"));
	}
	if (RowNameString.StartsWith(TEXT("Weapon_")))
	{
		return ResolveTradeInfoFromTable<FLSWeaponRow>(Settings->WeaponTable, ItemRowName, TEXT("Weapon"));
	}
	if (RowNameString.StartsWith(TEXT("Armor_")))
	{
		return ResolveTradeInfoFromTable<FLSArmorRow>(Settings->ArmorTable, ItemRowName, TEXT("Armor"));
	}
	if (RowNameString.StartsWith(TEXT("Item_")))
	{
		Info = ResolveTradeInfoFromTable<FLSItemRow>(Settings->ItemTable, ItemRowName, TEXT("Item"));
		if (Info.bValid)
		{
			UDataTable* ItemTable = Settings->ItemTable.LoadSynchronous();
			const FLSItemRow* Row = ItemTable ? ItemTable->FindRow<FLSItemRow>(ItemRowName, TEXT("ResolveItemTradeInfo")) : nullptr;
			Info.ItemType = Row ? Row->Item_Type : 0;
		}
		return Info;
	}

	UE_LOG(LogLS, Warning, TEXT("[Inventory] Unknown item row prefix for trade info: %s"), *ItemRowName.ToString());
	return Info;
}

ELSEquipmentSlot ResolveEquipmentSlotType(const FName ItemRowName)
{
	if (ItemRowName.IsNone())
	{
		return ELSEquipmentSlot::Count;
	}

	const FString RowNameString = ItemRowName.ToString();
	if (RowNameString.StartsWith(TEXT("Weapon_")))
	{
		return ELSEquipmentSlot::Weapon;
	}

	// 무기가 아니면 방어구만 장착 가능. 방어구 슬롯 타입은 Item_Equipment 필드가 단일 출처다.
	if (!RowNameString.StartsWith(TEXT("Armor_")))
	{
		return ELSEquipmentSlot::Count;
	}

	const ULSDropSettings* Settings = GetDefault<ULSDropSettings>();
	UDataTable* ArmorTable = Settings ? Settings->ArmorTable.LoadSynchronous() : nullptr;
	const FLSArmorRow* Row = ArmorTable ? ArmorTable->FindRow<FLSArmorRow>(ItemRowName, TEXT("ResolveEquipmentSlotType")) : nullptr;
	if (!Row)
	{
		UE_LOG(LogLS, Warning, TEXT("[Inventory] Armor row missing for equipment type: %s"), *ItemRowName.ToString());
		return ELSEquipmentSlot::Count;
	}

	if (Row->Item_Equipment == TEXT("Processor"))
	{
		return ELSEquipmentSlot::Processor;
	}
	if (Row->Item_Equipment == TEXT("Core"))
	{
		return ELSEquipmentSlot::Core;
	}
	if (Row->Item_Equipment == TEXT("Actuator"))
	{
		return ELSEquipmentSlot::Actuator;
	}
	if (Row->Item_Equipment == TEXT("Frame"))
	{
		return ELSEquipmentSlot::Frame;
	}

	UE_LOG(LogLS, Warning, TEXT("[Inventory] Unknown armor Item_Equipment '%s' for %s"), *Row->Item_Equipment, *ItemRowName.ToString());
	return ELSEquipmentSlot::Count;
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

FString ResolveIconAssetNameFromRowName(const FName ItemRowName)
{
	const FString RowNameString = ItemRowName.ToString();

	// 칩이 아니면 행 이름을 그대로 에셋명으로 사용(기존 동작 유지).
	if (!RowNameString.StartsWith(TEXT("Chip_")))
	{
		return RowNameString;
	}

	// Chip_{Grade}_{Func} 의 마지막 토큰(기능)으로 기능별 아이콘을 선택한다.
	FString FunctionToken;
	RowNameString.Split(TEXT("_"), nullptr, &FunctionToken, ESearchCase::CaseSensitive, ESearchDir::FromEnd);

	if (const FString* IconName = GetChipFunctionIconNames().Find(FunctionToken))
	{
		return *IconName;
	}

	UE_LOG(LogLS, Warning, TEXT("[Inventory] Cannot resolve chip icon from row name '%s' (unknown function token '%s')."), *RowNameString, *FunctionToken);
	return RowNameString;
}

void EnsureSlotIndex(TArray<FLSSessionItem>& Slots, const int32 SlotIndex)
{
	while (Slots.Num() <= SlotIndex)
	{
		Slots.Add(MakeEmptyItem());
	}
}

bool TryAddItemsToSlotArray(TArray<FLSSessionItem>& Slots, const FName ItemRowName, int32 Amount, const int32 MaxSlotCount, const TArray<FLSChipResolvedStat>& ChipStats, FLSSessionItem& OutRemainingItem)
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
		OutRemainingItem.ChipStats = ChipStats;
		return false;
	}

	// 확정 스탯 스냅샷을 가진 아이템(칩)은 개체별로 고유하므로 다른 슬롯과 절대 합치지 않는다.
	const bool bHasInstanceStats = ChipStats.Num() > 0;
	const int32 OriginalAmount = Amount;
	const int32 MaxStack = ResolveItemMaxStack(ItemRowName, TEXT("TryAddItemsToSlotArray"));
	const int32 ExistingSlotLimit = FMath::Min(Slots.Num(), MaxSlotCount);

	if (!bHasInstanceStats)
	{
		for (int32 SlotIndex = 0; SlotIndex < ExistingSlotLimit; ++SlotIndex)
		{
			FLSSessionItem& Slot = Slots[SlotIndex];
			if (Amount <= 0)
			{
				break;
			}

			if (Slot.ItemRowName != ItemRowName || Slot.ChipStats.Num() > 0 || Slot.Amount >= MaxStack)
			{
				continue;
			}

			const int32 AddAmount = FMath::Min(Amount, MaxStack - Slot.Amount);
			Slot.Amount += AddAmount;
			Amount -= AddAmount;
		}
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
		Slot.ChipStats = ChipStats;
		Amount -= Slot.Amount;
	}

	while (Amount > 0 && Slots.Num() < MaxSlotCount)
	{
		FLSSessionItem NewSlot;
		NewSlot.ItemRowName = ItemRowName;
		NewSlot.Amount = FMath::Min(Amount, MaxStack);
		NewSlot.ChipStats = ChipStats;
		Slots.Add(NewSlot);
		Amount -= NewSlot.Amount;
	}

	if (Amount > 0)
	{
		OutRemainingItem.ItemRowName = ItemRowName;
		OutRemainingItem.Amount = Amount;
		OutRemainingItem.ChipStats = ChipStats;
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
		TryAddItemsToSlotArray(Slots, ItemRowName, Amount, MAX_int32, /*ChipStats=*/TArray<FLSChipResolvedStat>(), RemainingItem);
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
		NormalizedSlot.ChipStats = OldSlot.ChipStats;
		Slots.Add(NormalizedSlot);

		const int32 OverflowAmount = OldSlot.Amount - NormalizedSlot.Amount;
		if (OverflowAmount > 0)
		{
			FLSSessionItem RemainingItem;
			TryAddItemsToSlotArray(Slots, OldSlot.ItemRowName, OverflowAmount, MAX_int32, OldSlot.ChipStats, RemainingItem);
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

		if (Slot.ItemRowName != ItemRowName || Slot.ChipStats.Num() > 0 || Slot.Amount <= 0)
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
	TArray<FLSSessionItem> InstanceItems;
	for (const FLSSessionItem& Slot : Slots)
	{
		if (IsFilled(Slot))
		{
			if (Slot.ChipStats.Num() > 0)
			{
				InstanceItems.Add(Slot);
			}
			else
			{
				AmountByRowName.FindOrAdd(Slot.ItemRowName) += Slot.Amount;
			}
		}
	}

	TArray<FLSSessionItem> MergedItems;
	MergedItems.Reserve(AmountByRowName.Num());
	for (const TPair<FName, int32>& Pair : AmountByRowName)
	{
		MergedItems.Add({ Pair.Key, Pair.Value });
	}
	MergedItems.Append(InstanceItems);

	MergedItems.Sort([](const FLSSessionItem& Left, const FLSSessionItem& Right)
	{
		const int32 LeftSortKey = ResolveItemSortKey(Left.ItemRowName);
		const int32 RightSortKey = ResolveItemSortKey(Right.ItemRowName);
		return LeftSortKey != RightSortKey ? LeftSortKey < RightSortKey : Left.ItemRowName.LexicalLess(Right.ItemRowName);
	});

	Slots.Reset();
	for (const FLSSessionItem& MergedItem : MergedItems)
	{
		FLSSessionItem IgnoredRemainingItem;
		TryAddItemsToSlotArray(Slots, MergedItem.ItemRowName, MergedItem.Amount, MAX_int32, MergedItem.ChipStats, IgnoredRemainingItem);
	}

	while (Slots.Num() < OriginalSlotCount)
	{
		Slots.Add(MakeEmptyItem());
	}
}

bool MoveEquipmentSlotBetweenArrays(TArray<FLSSessionItem>& FromSlots, const int32 FromIndex, const bool bFromEquipment, TArray<FLSSessionItem>& ToSlots, const int32 ToIndex, const bool bToEquipment, const int32 ToMaxSlotCount)
{
	// 장비끼리는 슬롯마다 타입이 고정이라 교환이 성립하지 않고, 둘 다 아니면 일반 슬롯 이동을 써야 한다.
	if (bFromEquipment == bToEquipment)
	{
		UE_LOG(LogLS, Warning, TEXT("[Inventory] Cannot move equipment slot with invalid area pair. FromEquipment=%d ToEquipment=%d"),
			bFromEquipment ? 1 : 0, bToEquipment ? 1 : 0);
		return false;
	}

	if (!FromSlots.IsValidIndex(FromIndex) || !IsFilled(FromSlots[FromIndex]) || ToIndex < 0)
	{
		return false;
	}

	// 장착: 장비 슬롯에 들어갈 아이템 타입이 그 슬롯 타입(=인덱스)과 일치해야 한다.
	if (bToEquipment)
	{
		if (ToIndex >= EquipmentSlotCount)
		{
			return false;
		}

		const FName SourceRow = FromSlots[FromIndex].ItemRowName;
		if (ResolveEquipmentSlotType(SourceRow) != static_cast<ELSEquipmentSlot>(ToIndex))
		{
			UE_LOG(LogLS, Warning, TEXT("[Inventory] Cannot equip '%s' into equipment slot %d because type mismatches."),
				*SourceRow.ToString(), ToIndex);
			return false;
		}
	}

	// 해제(교환): 대상 슬롯에 아이템이 있으면 스왑으로 그 아이템이 장비 슬롯(FromIndex)에 들어간다.
	// 들어갈 아이템 타입이 장비 슬롯 타입과 맞을 때만 허용한다(엉뚱한 아이템이 장착되는 것을 막는다).
	if (bFromEquipment && ToSlots.IsValidIndex(ToIndex) && IsFilled(ToSlots[ToIndex]))
	{
		const FName TargetRow = ToSlots[ToIndex].ItemRowName;
		if (ResolveEquipmentSlotType(TargetRow) != static_cast<ELSEquipmentSlot>(FromIndex))
		{
			UE_LOG(LogLS, Warning, TEXT("[Inventory] Cannot swap equipment slot %d with '%s' because type mismatches."),
				FromIndex, *TargetRow.ToString());
			return false;
		}
	}

	// 장비는 Item_Max=1이라 병합 없이 배치/스왑으로만 동작한다.
	return DropSlot(FromSlots, FromIndex, ToSlots, ToIndex, ToMaxSlotCount);
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

	if (FromSlot.ItemRowName == ToSlot.ItemRowName && FromSlot.ChipStats == ToSlot.ChipStats)
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

	if (InOutExternalItem.ItemRowName == ToSlot.ItemRowName && InOutExternalItem.ChipStats == ToSlot.ChipStats)
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

	if (FromSlot.ItemRowName == ToSlot.ItemRowName && FromSlot.ChipStats == ToSlot.ChipStats)
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
