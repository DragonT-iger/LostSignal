#include "Session/LSSessionSubsystem.h"
#include "Session/LSSessionSettings.h"
#include "Session/LSSaveSubsystem.h"
#include "Data/LSArmorRow.h"
#include "Data/LSChipRow.h"
#include "Data/LSDropSettings.h"
#include "Data/LSItemRow.h"
#include "Data/LSWeaponRow.h"
#include "Engine/DataTable.h"
#include "LostSignal.h"
#include "Kismet/GameplayStatics.h"

namespace
{
constexpr int32 DefaultMaxInventorySlotCount = 10;

bool IsFilledSessionSlot(const FLSSessionItem& Item)
{
	return !Item.ItemRowName.IsNone() && Item.Amount > 0;
}

FLSSessionItem MakeEmptySessionSlot()
{
	return FLSSessionItem();
}

int32 ResolveItemMaxStackForSession(const FName ItemRowName)
{
	const ULSDropSettings* Settings = GetDefault<ULSDropSettings>();
	if (!Settings || ItemRowName.IsNone())
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] Cannot resolve max stack. Row=%s"), *ItemRowName.ToString());
		return 1;
	}

	const FString RowNameString = ItemRowName.ToString();
	int32 MaxStack = 1;

	if (RowNameString.StartsWith(TEXT("Chip_")))
	{
		UDataTable* Table = Settings->ChipTable.LoadSynchronous();
		const FLSChipRow* Row = Table ? Table->FindRow<FLSChipRow>(ItemRowName, TEXT("ResolveItemMaxStackForSession")) : nullptr;
		MaxStack = Row ? Row->Item_Max : 1;
		if (!Row) UE_LOG(LogLS, Warning, TEXT("[Session] Chip row missing for max stack: %s"), *ItemRowName.ToString());
	}
	else if (RowNameString.StartsWith(TEXT("Weapon_")))
	{
		UDataTable* Table = Settings->WeaponTable.LoadSynchronous();
		const FLSWeaponRow* Row = Table ? Table->FindRow<FLSWeaponRow>(ItemRowName, TEXT("ResolveItemMaxStackForSession")) : nullptr;
		MaxStack = Row ? Row->Item_Max : 1;
		if (!Row) UE_LOG(LogLS, Warning, TEXT("[Session] Weapon row missing for max stack: %s"), *ItemRowName.ToString());
	}
	else if (RowNameString.StartsWith(TEXT("Armor_")))
	{
		UDataTable* Table = Settings->ArmorTable.LoadSynchronous();
		const FLSArmorRow* Row = Table ? Table->FindRow<FLSArmorRow>(ItemRowName, TEXT("ResolveItemMaxStackForSession")) : nullptr;
		MaxStack = Row ? Row->Item_Max : 1;
		if (!Row) UE_LOG(LogLS, Warning, TEXT("[Session] Armor row missing for max stack: %s"), *ItemRowName.ToString());
	}
	else if (RowNameString.StartsWith(TEXT("Item_")))
	{
		UDataTable* Table = Settings->ItemTable.LoadSynchronous();
		const FLSItemRow* Row = Table ? Table->FindRow<FLSItemRow>(ItemRowName, TEXT("ResolveItemMaxStackForSession")) : nullptr;
		MaxStack = Row ? Row->Item_Max : 1;
		if (!Row) UE_LOG(LogLS, Warning, TEXT("[Session] Item row missing for max stack: %s"), *ItemRowName.ToString());
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] Unknown item row prefix for max stack: %s"), *ItemRowName.ToString());
	}

	if (MaxStack <= 0)
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] Invalid Item_Max for %s: %d. Falling back to 1."), *ItemRowName.ToString(), MaxStack);
		return 1;
	}

	return MaxStack;
}

void AddItemsToSlotArraySession(TArray<FLSSessionItem>& Slots, const FName ItemRowName, int32 Amount)
{
	if (ItemRowName.IsNone() || Amount <= 0)
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] Cannot add invalid session item. Row=%s Amount=%d"), *ItemRowName.ToString(), Amount);
		return;
	}

	const int32 MaxStack = ResolveItemMaxStackForSession(ItemRowName);
	for (FLSSessionItem& Slot : Slots)
	{
		if (Amount <= 0)
		{
			return;
		}

		if (Slot.ItemRowName != ItemRowName || Slot.Amount >= MaxStack)
		{
			continue;
		}

		const int32 AddAmount = FMath::Min(Amount, MaxStack - Slot.Amount);
		Slot.Amount += AddAmount;
		Amount -= AddAmount;
	}

	for (FLSSessionItem& Slot : Slots)
	{
		if (Amount <= 0)
		{
			return;
		}

		if (IsFilledSessionSlot(Slot))
		{
			continue;
		}

		Slot.ItemRowName = ItemRowName;
		Slot.Amount = FMath::Min(Amount, MaxStack);
		Amount -= Slot.Amount;
	}

	while (Amount > 0)
	{
		FLSSessionItem NewSlot;
		NewSlot.ItemRowName = ItemRowName;
		NewSlot.Amount = FMath::Min(Amount, MaxStack);
		Slots.Add(NewSlot);
		Amount -= NewSlot.Amount;
	}
}

bool TryAddItemsToSlotArraySession(TArray<FLSSessionItem>& Slots, const FName ItemRowName, int32 Amount, const int32 MaxSlotCount, FLSSessionItem& OutRemainingItem)
{
	OutRemainingItem = FLSSessionItem();
	if (ItemRowName.IsNone() || Amount <= 0)
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] Cannot add invalid session item. Row=%s Amount=%d"), *ItemRowName.ToString(), Amount);
		return false;
	}

	if (MaxSlotCount <= 0)
	{
		OutRemainingItem.ItemRowName = ItemRowName;
		OutRemainingItem.Amount = Amount;
		return false;
	}

	const int32 OriginalAmount = Amount;
	const int32 MaxStack = ResolveItemMaxStackForSession(ItemRowName);
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

		if (IsFilledSessionSlot(Slot))
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

int32 FindRowOrderSession(UDataTable* Table, const FName RowName)
{
	if (!Table)
	{
		return MAX_int32 / 2;
	}

	const TArray<FName> RowNames = Table->GetRowNames();
	const int32 RowIndex = RowNames.IndexOfByKey(RowName);
	return RowIndex == INDEX_NONE ? MAX_int32 / 2 : RowIndex;
}

int32 ResolveItemSortKeyForSession(const FName ItemRowName)
{
	const ULSDropSettings* Settings = GetDefault<ULSDropSettings>();
	if (!Settings || ItemRowName.IsNone())
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] Cannot resolve sort key. Row=%s"), *ItemRowName.ToString());
		return MAX_int32;
	}

	const FString RowNameString = ItemRowName.ToString();
	if (RowNameString.StartsWith(TEXT("Chip_")))
	{
		return FindRowOrderSession(Settings->ChipTable.LoadSynchronous(), ItemRowName);
	}

	if (RowNameString.StartsWith(TEXT("Weapon_")))
	{
		return 100000 + FindRowOrderSession(Settings->WeaponTable.LoadSynchronous(), ItemRowName);
	}

	if (RowNameString.StartsWith(TEXT("Armor_")))
	{
		return 200000 + FindRowOrderSession(Settings->ArmorTable.LoadSynchronous(), ItemRowName);
	}

	if (RowNameString.StartsWith(TEXT("Item_")))
	{
		return 300000 + FindRowOrderSession(Settings->ItemTable.LoadSynchronous(), ItemRowName);
	}

	UE_LOG(LogLS, Warning, TEXT("[Session] Unknown item row prefix for sort key: %s"), *ItemRowName.ToString());
	return MAX_int32;
}

void SortAndCompactSlotArraySession(TArray<FLSSessionItem>& Slots)
{
	const int32 OriginalSlotCount = Slots.Num();
	TMap<FName, int32> AmountByRowName;
	for (const FLSSessionItem& Slot : Slots)
	{
		if (!IsFilledSessionSlot(Slot))
		{
			continue;
		}

		AmountByRowName.FindOrAdd(Slot.ItemRowName) += Slot.Amount;
	}

	TArray<FLSSessionItem> MergedItems;
	MergedItems.Reserve(AmountByRowName.Num());
	for (const TPair<FName, int32>& Pair : AmountByRowName)
	{
		MergedItems.Add({ Pair.Key, Pair.Value });
	}

	MergedItems.Sort([](const FLSSessionItem& Left, const FLSSessionItem& Right)
	{
		const int32 LeftSortKey = ResolveItemSortKeyForSession(Left.ItemRowName);
		const int32 RightSortKey = ResolveItemSortKeyForSession(Right.ItemRowName);
		if (LeftSortKey != RightSortKey)
		{
			return LeftSortKey < RightSortKey;
		}

		return Left.ItemRowName.LexicalLess(Right.ItemRowName);
	});

	Slots.Reset();
	for (const FLSSessionItem& MergedItem : MergedItems)
	{
		AddItemsToSlotArraySession(Slots, MergedItem.ItemRowName, MergedItem.Amount);
	}

	while (Slots.Num() < OriginalSlotCount)
	{
		Slots.Add(MakeEmptySessionSlot());
	}
}

void RemoveItemsFromSlotArraySession(TArray<FLSSessionItem>& Slots, const FName ItemRowName, int32 Amount)
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
		if (Slot.Amount <= 0)
		{
			Slot = MakeEmptySessionSlot();
		}
	}
}

void EnsureSlotIndex(TArray<FLSSessionItem>& Slots, const int32 SlotIndex)
{
	while (Slots.Num() <= SlotIndex)
	{
		Slots.Add(MakeEmptySessionSlot());
	}
}
}

void ULSSessionSubsystem::StartRaid(const TArray<FLSSessionItem>& Loadout)
{
	LoadoutSnapshot.Items = Loadout;
	SessionInventory = Loadout;
	SessionSafeInventory.Empty();
	ConsumedItems.Empty();
	ResolvedItems.Empty();
	bRaidActive = true;

	if (ULSSaveSubsystem* SaveSub = GetGameInstance()->GetSubsystem<ULSSaveSubsystem>())
	{
		SessionSafeInventory = SaveSub->GetSafeStash();
		SaveSub->BeginRaidSave(LoadoutSnapshot.Items);
	}

	UE_LOG(LogLS, Log, TEXT("[Session] Raid started with %d inventory slots."), SessionInventory.Num());
}

void ULSSessionSubsystem::EndRaid(ELSRaidResult Result)
{
	LastRaidResult = Result;
	ResolvedItems.Empty();
	bool bShouldSaveResolvedItems = false;

	switch (Result)
	{
	case ELSRaidResult::Extracted:
		ResolvedItems = SessionInventory;
		bShouldSaveResolvedItems = true;
		UE_LOG(LogLS, Log, TEXT("[Session] 탈출 성공 - 획득 아이템 %d종 보관"), ResolvedItems.Num());
		break;

	case ELSRaidResult::Quit:
		if (bAllowQuitRecovery)
		{
			ResolvedItems = BuildQuitRecovery();
			bShouldSaveResolvedItems = true;
			UE_LOG(LogLS, Log, TEXT("[Session] 탈주 - 장비 복구 %d종"), ResolvedItems.Num());
		}
		else
		{
			UE_LOG(LogLS, Log, TEXT("[Session] 탈주 - 장비 복구 비활성화, 전부 소실"));
		}
		break;

	case ELSRaidResult::Dead:
		UE_LOG(LogLS, Log, TEXT("[Session] 사망 - 전부 소실"));
		break;
	}

	// 스태시에 저장 (레벨 전환 전에 처리)
	if (ULSSaveSubsystem* SaveSub = GetGameInstance()->GetSubsystem<ULSSaveSubsystem>())
	{
		if (Result == ELSRaidResult::Quit)
		{
			ResolvedItems = BuildQuitRecovery();
			bShouldSaveResolvedItems = true;
		}
		else if (Result == ELSRaidResult::Dead)
		{
			ResolvedItems.Empty();
			bShouldSaveResolvedItems = true;
		}

		if (bShouldSaveResolvedItems)
		{
			SaveSub->ReplaceStash(ResolvedItems);
			if (Result == ELSRaidResult::Extracted || Result == ELSRaidResult::Dead)
			{
				SaveSub->ReplaceSafeStash(SessionSafeInventory);
			}
		}

		SaveSub->ClearRaidSave();
	}

	bRaidActive = false;

	// 결과 레벨로 전환
	const ULSSessionSettings* Settings = GetDefault<ULSSessionSettings>();
	if (!Settings->ResultLevel.IsNull())
	{
		UGameplayStatics::OpenLevelBySoftObjectPtr(this, Settings->ResultLevel);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] ResultLevel 미설정 - 프로젝트 설정 > LS Session Settings 확인"));
	}
}

void ULSSessionSubsystem::AddSessionItem(FName ItemRowName, int32 Amount)
{
	FLSSessionItem IgnoredRemainingItem;
	TryAddSessionItem(ItemRowName, Amount, IgnoredRemainingItem);
}

bool ULSSessionSubsystem::TryAddSessionItem(FName ItemRowName, int32 Amount, FLSSessionItem& OutRemainingItem)
{
	return TryAddItemsToSlotArraySession(SessionInventory, ItemRowName, Amount, GetMaxInventorySlotCount(), OutRemainingItem);
}

void ULSSessionSubsystem::SortSessionInventory()
{
	SortAndCompactSlotArraySession(SessionInventory);
	UE_LOG(LogLS, Log, TEXT("[Session] Session inventory sorted and compacted. Total slots: %d"), SessionInventory.Num());
}

bool ULSSessionSubsystem::SwapSessionInventorySlots(const int32 FromIndex, const int32 ToIndex)
{
	return SwapSessionSlots(ELSInventorySlotArea::Inventory, FromIndex, ELSInventorySlotArea::Inventory, ToIndex);
}

bool ULSSessionSubsystem::MoveSessionInventorySlot(const int32 FromIndex, const int32 ToIndex)
{
	return MoveSessionSlot(ELSInventorySlotArea::Inventory, FromIndex, ELSInventorySlotArea::Inventory, ToIndex);
}

bool ULSSessionSubsystem::SwapSessionSlots(const ELSInventorySlotArea FromArea, const int32 FromIndex, const ELSInventorySlotArea ToArea, const int32 ToIndex)
{
	TArray<FLSSessionItem>* FromSlots = FromArea == ELSInventorySlotArea::Safe ? &SessionSafeInventory : &SessionInventory;
	TArray<FLSSessionItem>* ToSlots = ToArea == ELSInventorySlotArea::Safe ? &SessionSafeInventory : &SessionInventory;

	if (!FromSlots->IsValidIndex(FromIndex) || !IsFilledSessionSlot((*FromSlots)[FromIndex]) || ToIndex < 0)
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] Cannot swap slots. FromArea=%d From=%d ToArea=%d To=%d"),
			static_cast<int32>(FromArea), FromIndex, static_cast<int32>(ToArea), ToIndex);
		return false;
	}

	if (FromSlots == ToSlots && FromIndex == ToIndex)
	{
		return true;
	}

	EnsureSlotIndex(*ToSlots, ToIndex);
	Swap((*FromSlots)[FromIndex], (*ToSlots)[ToIndex]);
	return true;
}

bool ULSSessionSubsystem::MoveSessionSlot(const ELSInventorySlotArea FromArea, const int32 FromIndex, const ELSInventorySlotArea ToArea, const int32 ToIndex)
{
	if (FromArea != ToArea)
	{
		return SwapSessionSlots(FromArea, FromIndex, ToArea, ToIndex);
	}

	TArray<FLSSessionItem>& Slots = FromArea == ELSInventorySlotArea::Safe ? SessionSafeInventory : SessionInventory;
	if (!Slots.IsValidIndex(FromIndex) || !IsFilledSessionSlot(Slots[FromIndex]))
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] Cannot move inventory slot because FromIndex is invalid: %d"), FromIndex);
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

	UE_LOG(LogLS, Log, TEXT("[Session] Moved inventory slot %d(%s) -> %d"),
		FromIndex,
		*MovingItem.ItemRowName.ToString(),
		ClampedTargetIndex);
	return true;
}

bool ULSSessionSubsystem::DropSessionSlot(const ELSInventorySlotArea FromArea, const int32 FromIndex, const ELSInventorySlotArea ToArea, const int32 ToIndex)
{
	TArray<FLSSessionItem>* FromSlots = FromArea == ELSInventorySlotArea::Safe ? &SessionSafeInventory : &SessionInventory;
	TArray<FLSSessionItem>* ToSlots = ToArea == ELSInventorySlotArea::Safe ? &SessionSafeInventory : &SessionInventory;

	if (!FromSlots->IsValidIndex(FromIndex) || !IsFilledSessionSlot((*FromSlots)[FromIndex]) || ToIndex < 0)
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] Cannot drop slot. FromArea=%d From=%d ToArea=%d To=%d"),
			static_cast<int32>(FromArea), FromIndex, static_cast<int32>(ToArea), ToIndex);
		return false;
	}

	if (FromSlots == ToSlots && FromIndex == ToIndex)
	{
		return true;
	}

	EnsureSlotIndex(*ToSlots, ToIndex);

	FLSSessionItem& FromSlot = (*FromSlots)[FromIndex];
	FLSSessionItem& ToSlot = (*ToSlots)[ToIndex];

	if (!IsFilledSessionSlot(ToSlot))
	{
		ToSlot = FromSlot;
		FromSlot = MakeEmptySessionSlot();
		return true;
	}

	if (FromSlot.ItemRowName == ToSlot.ItemRowName)
	{
		const int32 MaxStack = ResolveItemMaxStackForSession(FromSlot.ItemRowName);
		const int32 AddAmount = FMath::Min(FromSlot.Amount, MaxStack - ToSlot.Amount);
		if (AddAmount <= 0)
		{
			return false;
		}

		ToSlot.Amount += AddAmount;
		FromSlot.Amount -= AddAmount;
		if (FromSlot.Amount <= 0)
		{
			FromSlot = MakeEmptySessionSlot();
		}
		return true;
	}

	Swap(FromSlot, ToSlot);
	return true;
}

bool ULSSessionSubsystem::DropExternalItemToSessionSlot(FLSSessionItem& InOutExternalItem, const ELSInventorySlotArea ToArea, const int32 ToIndex)
{
	TArray<FLSSessionItem>* ToSlots = ToArea == ELSInventorySlotArea::Safe ? &SessionSafeInventory : &SessionInventory;
	if (!IsFilledSessionSlot(InOutExternalItem) || ToIndex < 0)
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] Cannot drop external item. ToArea=%d To=%d Row=%s Amount=%d"),
			static_cast<int32>(ToArea),
			ToIndex,
			*InOutExternalItem.ItemRowName.ToString(),
			InOutExternalItem.Amount);
		return false;
	}

	if (ToArea == ELSInventorySlotArea::Inventory && ToIndex >= GetMaxInventorySlotCount())
	{
		return false;
	}

	EnsureSlotIndex(*ToSlots, ToIndex);
	FLSSessionItem& ToSlot = (*ToSlots)[ToIndex];

	if (!IsFilledSessionSlot(ToSlot))
	{
		ToSlot = InOutExternalItem;
		InOutExternalItem = MakeEmptySessionSlot();
		return true;
	}

	if (InOutExternalItem.ItemRowName == ToSlot.ItemRowName)
	{
		const int32 MaxStack = ResolveItemMaxStackForSession(InOutExternalItem.ItemRowName);
		const int32 AddAmount = FMath::Min(InOutExternalItem.Amount, MaxStack - ToSlot.Amount);
		if (AddAmount <= 0)
		{
			return false;
		}

		ToSlot.Amount += AddAmount;
		InOutExternalItem.Amount -= AddAmount;
		if (InOutExternalItem.Amount <= 0)
		{
			InOutExternalItem = MakeEmptySessionSlot();
		}
		return true;
	}

	Swap(InOutExternalItem, ToSlot);
	return true;
}

bool ULSSessionSubsystem::GetSessionSlotItem(const ELSInventorySlotArea SlotArea, const int32 SlotIndex, FLSSessionItem& OutItem) const
{
	const TArray<FLSSessionItem>& Slots = SlotArea == ELSInventorySlotArea::Safe ? SessionSafeInventory : SessionInventory;
	if (!Slots.IsValidIndex(SlotIndex) || !IsFilledSessionSlot(Slots[SlotIndex]))
	{
		return false;
	}

	OutItem = Slots[SlotIndex];
	return true;
}

bool ULSSessionSubsystem::ClearSessionSlot(const ELSInventorySlotArea SlotArea, const int32 SlotIndex)
{
	TArray<FLSSessionItem>& Slots = SlotArea == ELSInventorySlotArea::Safe ? SessionSafeInventory : SessionInventory;
	if (!Slots.IsValidIndex(SlotIndex) || !IsFilledSessionSlot(Slots[SlotIndex]))
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] Cannot clear slot. Area=%d Index=%d"),
			static_cast<int32>(SlotArea), SlotIndex);
		return false;
	}

	Slots[SlotIndex] = MakeEmptySessionSlot();
	return true;
}

bool ULSSessionSubsystem::ReplaceSessionSlotItem(const ELSInventorySlotArea SlotArea, const int32 SlotIndex, const FLSSessionItem& NewItem, FLSSessionItem& OutPreviousItem)
{
	OutPreviousItem = FLSSessionItem();
	if (SlotIndex < 0)
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] Cannot replace slot because index is invalid. Area=%d Index=%d"),
			static_cast<int32>(SlotArea), SlotIndex);
		return false;
	}

	if (SlotArea == ELSInventorySlotArea::Inventory && SlotIndex >= GetMaxInventorySlotCount())
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] Cannot replace inventory slot because index exceeds max. Index=%d"), SlotIndex);
		return false;
	}

	TArray<FLSSessionItem>& Slots = SlotArea == ELSInventorySlotArea::Safe ? SessionSafeInventory : SessionInventory;
	EnsureSlotIndex(Slots, SlotIndex);
	OutPreviousItem = Slots[SlotIndex];
	Slots[SlotIndex] = NewItem;
	return true;
}

int32 ULSSessionSubsystem::GetMaxInventorySlotCount() const
{
	return DefaultMaxInventorySlotCount;
}

void ULSSessionSubsystem::ConsumeItem(FName ItemRowName, int32 Amount)
{
	if (ItemRowName.IsNone() || Amount <= 0) return;

	for (FLSSessionItem& Item : ConsumedItems)
	{
		if (Item.ItemRowName == ItemRowName)
		{
			Item.Amount += Amount;
			if (ULSSaveSubsystem* SaveSub = GetGameInstance()->GetSubsystem<ULSSaveSubsystem>())
			{
				SaveSub->UpdateRaidConsumedItems(ConsumedItems);
			}
			return;
		}
	}

	ConsumedItems.Add({ ItemRowName, Amount });
	if (ULSSaveSubsystem* SaveSub = GetGameInstance()->GetSubsystem<ULSSaveSubsystem>())
	{
		SaveSub->UpdateRaidConsumedItems(ConsumedItems);
	}
}

TArray<FLSSessionItem> ULSSessionSubsystem::BuildQuitRecovery() const
{
	// 출발 장비에서 소모된 수량을 차감해 반환
	TArray<FLSSessionItem> Recovery = LoadoutSnapshot.Items;

	for (const FLSSessionItem& Consumed : ConsumedItems)
	{
		RemoveItemsFromSlotArraySession(Recovery, Consumed.ItemRowName, Consumed.Amount);
	}

	// 수량이 0이 된 항목 제거
	return Recovery;
}
