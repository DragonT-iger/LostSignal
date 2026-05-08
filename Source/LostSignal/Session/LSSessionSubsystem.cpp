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

void AddItemsToSlotArray(TArray<FLSSessionItem>& Slots, const FName ItemRowName, int32 Amount)
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

	while (Amount > 0)
	{
		FLSSessionItem NewSlot;
		NewSlot.ItemRowName = ItemRowName;
		NewSlot.Amount = FMath::Min(Amount, MaxStack);
		Slots.Add(NewSlot);
		Amount -= NewSlot.Amount;
	}
}

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
		return FindRowOrder(Settings->ChipTable.LoadSynchronous(), ItemRowName);
	}

	if (RowNameString.StartsWith(TEXT("Weapon_")))
	{
		return 100000 + FindRowOrder(Settings->WeaponTable.LoadSynchronous(), ItemRowName);
	}

	if (RowNameString.StartsWith(TEXT("Armor_")))
	{
		return 200000 + FindRowOrder(Settings->ArmorTable.LoadSynchronous(), ItemRowName);
	}

	if (RowNameString.StartsWith(TEXT("Item_")))
	{
		return 300000 + FindRowOrder(Settings->ItemTable.LoadSynchronous(), ItemRowName);
	}

	UE_LOG(LogLS, Warning, TEXT("[Session] Unknown item row prefix for sort key: %s"), *ItemRowName.ToString());
	return MAX_int32;
}

void SortAndCompactSlotArray(TArray<FLSSessionItem>& Slots)
{
	TMap<FName, int32> AmountByRowName;
	for (const FLSSessionItem& Slot : Slots)
	{
		if (Slot.ItemRowName.IsNone() || Slot.Amount <= 0)
		{
			UE_LOG(LogLS, Warning, TEXT("[Session] Skipping invalid slot while sorting. Row=%s Amount=%d"), *Slot.ItemRowName.ToString(), Slot.Amount);
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
		AddItemsToSlotArray(Slots, MergedItem.ItemRowName, MergedItem.Amount);
	}
}
}

void ULSSessionSubsystem::StartRaid(const TArray<FLSSessionItem>& Loadout)
{
	LoadoutSnapshot.Items = Loadout;
	SessionInventory = Loadout;
	ConsumedItems.Empty();
	ResolvedItems.Empty();
	bRaidActive = true;

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
	if (bShouldSaveResolvedItems)
	{
		if (ULSSaveSubsystem* SaveSub = GetGameInstance()->GetSubsystem<ULSSaveSubsystem>())
		{
			SaveSub->ReplaceStash(ResolvedItems);
		}
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
	AddItemsToSlotArray(SessionInventory, ItemRowName, Amount);
}

void ULSSessionSubsystem::SortSessionInventory()
{
	SortAndCompactSlotArray(SessionInventory);
	UE_LOG(LogLS, Log, TEXT("[Session] Session inventory sorted and compacted. Total slots: %d"), SessionInventory.Num());
}

bool ULSSessionSubsystem::SwapSessionInventorySlots(const int32 FromIndex, const int32 ToIndex)
{
	if (!SessionInventory.IsValidIndex(FromIndex))
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] Cannot swap inventory slots because FromIndex is invalid: %d"), FromIndex);
		return false;
	}

	if (FromIndex == ToIndex)
	{
		return true;
	}

	if (SessionInventory.IsValidIndex(ToIndex))
	{
		const FName FromItemRowName = SessionInventory[FromIndex].ItemRowName;
		const FName ToItemRowName = SessionInventory[ToIndex].ItemRowName;
		SessionInventory.Swap(FromIndex, ToIndex);
		UE_LOG(LogLS, Log, TEXT("[Session] Swapped inventory slots %d(%s) <-> %d(%s)"),
			FromIndex,
			*FromItemRowName.ToString(),
			ToIndex,
			*ToItemRowName.ToString());
		return true;
	}

	return MoveSessionInventorySlot(FromIndex, ToIndex);
}

bool ULSSessionSubsystem::MoveSessionInventorySlot(const int32 FromIndex, const int32 ToIndex)
{
	if (!SessionInventory.IsValidIndex(FromIndex))
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] Cannot move inventory slot because FromIndex is invalid: %d"), FromIndex);
		return false;
	}

	if (FromIndex == ToIndex)
	{
		return true;
	}

	FLSSessionItem MovingItem = SessionInventory[FromIndex];
	SessionInventory.RemoveAt(FromIndex);

	const int32 ClampedTargetIndex = FMath::Clamp(ToIndex, 0, SessionInventory.Num());
	SessionInventory.Insert(MovingItem, ClampedTargetIndex);

	UE_LOG(LogLS, Log, TEXT("[Session] Moved inventory slot %d(%s) -> %d"),
		FromIndex,
		*MovingItem.ItemRowName.ToString(),
		ClampedTargetIndex);
	return true;
}

void ULSSessionSubsystem::ConsumeItem(FName ItemRowName, int32 Amount)
{
	if (ItemRowName.IsNone() || Amount <= 0) return;

	for (FLSSessionItem& Item : ConsumedItems)
	{
		if (Item.ItemRowName == ItemRowName)
		{
			Item.Amount += Amount;
			return;
		}
	}

	ConsumedItems.Add({ ItemRowName, Amount });
}

TArray<FLSSessionItem> ULSSessionSubsystem::BuildQuitRecovery() const
{
	// 출발 장비에서 소모된 수량을 차감해 반환
	TArray<FLSSessionItem> Recovery = LoadoutSnapshot.Items;

	for (const FLSSessionItem& Consumed : ConsumedItems)
	{
		for (FLSSessionItem& Item : Recovery)
		{
			if (Item.ItemRowName != Consumed.ItemRowName) continue;

			Item.Amount = FMath::Max(0, Item.Amount - Consumed.Amount);
			break;
		}
	}

	// 수량이 0이 된 항목 제거
	Recovery.RemoveAll([](const FLSSessionItem& Item) { return Item.Amount <= 0; });

	return Recovery;
}
