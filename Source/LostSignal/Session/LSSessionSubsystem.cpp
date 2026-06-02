#include "Session/LSSessionSubsystem.h"
#include "Session/LSSessionSettings.h"
#include "Session/LSSaveSubsystem.h"
#include "Inventory/LSInventorySlotUtils.h"
#include "LostSignal.h"
#include "Kismet/GameplayStatics.h"

namespace
{
constexpr int32 SessionDefaultMaxInventorySlotCount = 10;
}

void ULSSessionSubsystem::StartRaid(const TArray<FLSSessionItem>& Loadout)
{
	StartRaidInternal(Loadout, true);
}

void ULSSessionSubsystem::StartRaidClientMirror(const TArray<FLSSessionItem>& Loadout)
{
	StartRaidInternal(Loadout, false);
}

void ULSSessionSubsystem::MirrorRaidSessionState(const TArray<FLSSessionItem>& InventoryItems, const TArray<FLSSessionItem>& SafeItems)
{
	SessionInventory = InventoryItems;
	SessionSafeInventory = SafeItems;
	bRaidActive = true;
}

void ULSSessionSubsystem::ClearRaidSessionState()
{
	LoadoutSnapshot.Items.Reset();
	SessionInventory.Reset();
	SessionSafeInventory.Reset();
	ConsumedItems.Reset();
	ResolvedItems.Reset();
	bRaidActive = false;
	PendingRaidEntries.Reset();
	PendingRaidEntryIndex = 0;
}

void ULSSessionSubsystem::EnqueuePendingRaidEntry(
	const TArray<FLSSessionItem>& Inventory,
	const TArray<FLSSessionItem>& SafeInventory)
{
	FLSPendingRaidEntry& Entry = PendingRaidEntries.AddDefaulted_GetRef();
	Entry.Inventory = Inventory;
	Entry.SafeInventory = SafeInventory;
}

bool ULSSessionSubsystem::DequeuePendingRaidEntry(
	TArray<FLSSessionItem>& OutInventory,
	TArray<FLSSessionItem>& OutSafeInventory)
{
	if (PendingRaidEntryIndex >= PendingRaidEntries.Num())
	{
		return false;
	}
	const FLSPendingRaidEntry& Entry = PendingRaidEntries[PendingRaidEntryIndex++];
	OutInventory = Entry.Inventory;
	OutSafeInventory = Entry.SafeInventory;
	return true;
}

bool ULSSessionSubsystem::HasPendingRaidEntries() const
{
	return PendingRaidEntryIndex < PendingRaidEntries.Num();
}

void ULSSessionSubsystem::StartRaidInternal(const TArray<FLSSessionItem>& Loadout, const bool bPersistRaidSave)
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
		if (bPersistRaidSave)
		{
			SaveSub->BeginRaidSave(LoadoutSnapshot.Items);
		}
	}

	UE_LOG(LogLS, Log, TEXT("[Session] Raid started with %d inventory slots."), SessionInventory.Num());
}

void ULSSessionSubsystem::EndRaid(ELSRaidResult Result)
{
	LastRaidResult = Result;
	ResolvedItems.Empty();
	bool bShouldSaveResolvedItems = false;
	bool bShouldSaveSafeStash = false;

	switch (Result)
	{
	case ELSRaidResult::Extracted:
		ResolvedItems = SessionInventory;
		bShouldSaveResolvedItems = true;
		bShouldSaveSafeStash = true;
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
		ResolvedItems.Empty();
		bShouldSaveResolvedItems = true;
		bShouldSaveSafeStash = true;
		UE_LOG(LogLS, Log, TEXT("[Session] 사망 - 전부 소실"));
		break;
	}

	// 스태시에 저장 (레벨 전환 전에 처리)
	if (ULSSaveSubsystem* SaveSub = GetGameInstance()->GetSubsystem<ULSSaveSubsystem>())
	{
		if (bShouldSaveResolvedItems)
		{
			UE_LOG(LogLS, Log, TEXT("[Session] Applying raid result save. Result=%d InventorySlots=%d SafeSlots=%d"),
				static_cast<int32>(Result),
				ResolvedItems.Num(),
				SessionSafeInventory.Num());
			SaveSub->ReplaceInventory(ResolvedItems);
		}

		if (bShouldSaveSafeStash)
		{
			SaveSub->ReplaceSafeStash(SessionSafeInventory);
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
	TryAddSessionItem(ItemRowName, Amount, /*ChipStats=*/TArray<FLSChipResolvedStat>(), IgnoredRemainingItem);
}

bool ULSSessionSubsystem::TryAddSessionItem(FName ItemRowName, int32 Amount, const TArray<FLSChipResolvedStat>& ChipStats, FLSSessionItem& OutRemainingItem)
{
	return LSInventorySlotUtils::TryAddItemsToSlotArray(SessionInventory, ItemRowName, Amount, GetMaxInventorySlotCount(), ChipStats, OutRemainingItem);
}

void ULSSessionSubsystem::SortSessionInventory()
{
	LSInventorySlotUtils::SortAndCompactSlotArray(SessionInventory);
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
	if (FromArea == ELSInventorySlotArea::Warehouse || ToArea == ELSInventorySlotArea::Warehouse)
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] Warehouse slots are not part of raid session inventory."));
		return false;
	}

	TArray<FLSSessionItem>* FromSlots = FromArea == ELSInventorySlotArea::Safe ? &SessionSafeInventory : &SessionInventory;
	TArray<FLSSessionItem>* ToSlots = ToArea == ELSInventorySlotArea::Safe ? &SessionSafeInventory : &SessionInventory;

	if (!FromSlots->IsValidIndex(FromIndex) || !LSInventorySlotUtils::IsFilled((*FromSlots)[FromIndex]) || ToIndex < 0)
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] Cannot swap slots. FromArea=%d From=%d ToArea=%d To=%d"),
			static_cast<int32>(FromArea), FromIndex, static_cast<int32>(ToArea), ToIndex);
		return false;
	}

	const int32 ToMaxSlotCount = ToArea == ELSInventorySlotArea::Inventory ? GetMaxInventorySlotCount() : INDEX_NONE;
	return LSInventorySlotUtils::SwapSlots(*FromSlots, FromIndex, *ToSlots, ToIndex, ToMaxSlotCount);
}

bool ULSSessionSubsystem::MoveSessionSlot(const ELSInventorySlotArea FromArea, const int32 FromIndex, const ELSInventorySlotArea ToArea, const int32 ToIndex)
{
	if (FromArea == ELSInventorySlotArea::Warehouse || ToArea == ELSInventorySlotArea::Warehouse)
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] Warehouse slots are not part of raid session inventory."));
		return false;
	}

	if (FromArea != ToArea)
	{
		return SwapSessionSlots(FromArea, FromIndex, ToArea, ToIndex);
	}

	TArray<FLSSessionItem>& Slots = FromArea == ELSInventorySlotArea::Safe ? SessionSafeInventory : SessionInventory;
	if (!Slots.IsValidIndex(FromIndex) || !LSInventorySlotUtils::IsFilled(Slots[FromIndex]))
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] Cannot move inventory slot because FromIndex is invalid: %d"), FromIndex);
		return false;
	}

	return LSInventorySlotUtils::MoveSlotWithinArray(Slots, FromIndex, ToIndex);
}

bool ULSSessionSubsystem::DropSessionSlot(const ELSInventorySlotArea FromArea, const int32 FromIndex, const ELSInventorySlotArea ToArea, const int32 ToIndex)
{
	if (FromArea == ELSInventorySlotArea::Warehouse || ToArea == ELSInventorySlotArea::Warehouse)
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] Warehouse slots are not part of raid session inventory."));
		return false;
	}

	TArray<FLSSessionItem>* FromSlots = FromArea == ELSInventorySlotArea::Safe ? &SessionSafeInventory : &SessionInventory;
	TArray<FLSSessionItem>* ToSlots = ToArea == ELSInventorySlotArea::Safe ? &SessionSafeInventory : &SessionInventory;

	if (!FromSlots->IsValidIndex(FromIndex) || !LSInventorySlotUtils::IsFilled((*FromSlots)[FromIndex]) || ToIndex < 0)
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] Cannot drop slot. FromArea=%d From=%d ToArea=%d To=%d"),
			static_cast<int32>(FromArea), FromIndex, static_cast<int32>(ToArea), ToIndex);
		return false;
	}

	const int32 ToMaxSlotCount = ToArea == ELSInventorySlotArea::Inventory ? GetMaxInventorySlotCount() : INDEX_NONE;
	return LSInventorySlotUtils::DropSlot(*FromSlots, FromIndex, *ToSlots, ToIndex, ToMaxSlotCount);
}

bool ULSSessionSubsystem::DropExternalItemToSessionSlot(FLSSessionItem& InOutExternalItem, const ELSInventorySlotArea ToArea, const int32 ToIndex)
{
	if (ToArea == ELSInventorySlotArea::Warehouse)
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] Warehouse slots are not part of raid session inventory."));
		return false;
	}

	TArray<FLSSessionItem>* ToSlots = ToArea == ELSInventorySlotArea::Safe ? &SessionSafeInventory : &SessionInventory;
	if (!LSInventorySlotUtils::IsFilled(InOutExternalItem) || ToIndex < 0)
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] Cannot drop external item. ToArea=%d To=%d Row=%s Amount=%d"),
			static_cast<int32>(ToArea),
			ToIndex,
			*InOutExternalItem.ItemRowName.ToString(),
			InOutExternalItem.Amount);
		return false;
	}

	const int32 ToMaxSlotCount = ToArea == ELSInventorySlotArea::Inventory ? GetMaxInventorySlotCount() : INDEX_NONE;
	return LSInventorySlotUtils::DropExternalItemToSlot(InOutExternalItem, *ToSlots, ToIndex, ToMaxSlotCount);
}

bool ULSSessionSubsystem::GetSessionSlotItem(const ELSInventorySlotArea SlotArea, const int32 SlotIndex, FLSSessionItem& OutItem) const
{
	if (SlotArea == ELSInventorySlotArea::Warehouse)
	{
		return false;
	}

	const TArray<FLSSessionItem>& Slots = SlotArea == ELSInventorySlotArea::Safe ? SessionSafeInventory : SessionInventory;
	if (!Slots.IsValidIndex(SlotIndex) || !LSInventorySlotUtils::IsFilled(Slots[SlotIndex]))
	{
		return false;
	}

	OutItem = Slots[SlotIndex];
	return true;
}

bool ULSSessionSubsystem::ClearSessionSlot(const ELSInventorySlotArea SlotArea, const int32 SlotIndex)
{
	if (SlotArea == ELSInventorySlotArea::Warehouse)
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] Warehouse slots are not part of raid session inventory."));
		return false;
	}

	TArray<FLSSessionItem>& Slots = SlotArea == ELSInventorySlotArea::Safe ? SessionSafeInventory : SessionInventory;
	if (!Slots.IsValidIndex(SlotIndex) || !LSInventorySlotUtils::IsFilled(Slots[SlotIndex]))
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] Cannot clear slot. Area=%d Index=%d"),
			static_cast<int32>(SlotArea), SlotIndex);
		return false;
	}

	Slots[SlotIndex] = LSInventorySlotUtils::MakeEmptyItem();
	return true;
}

bool ULSSessionSubsystem::ReplaceSessionSlotItem(const ELSInventorySlotArea SlotArea, const int32 SlotIndex, const FLSSessionItem& NewItem, FLSSessionItem& OutPreviousItem)
{
	OutPreviousItem = FLSSessionItem();
	if (SlotArea == ELSInventorySlotArea::Warehouse)
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] Warehouse slots are not part of raid session inventory."));
		return false;
	}

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
	LSInventorySlotUtils::EnsureSlotIndex(Slots, SlotIndex);
	OutPreviousItem = Slots[SlotIndex];
	Slots[SlotIndex] = NewItem;
	return true;
}

int32 ULSSessionSubsystem::GetMaxInventorySlotCount() const
{
	return SessionDefaultMaxInventorySlotCount;
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
		LSInventorySlotUtils::RemoveItemsFromSlotArray(Recovery, Consumed.ItemRowName, Consumed.Amount);
	}

	// 수량이 0이 된 항목 제거
	return Recovery;
}
