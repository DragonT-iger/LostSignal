#include "Session/LSSessionSubsystem.h"
#include "Session/LSSaveSubsystem.h"
#include "Inventory/LSInventorySlotUtils.h"
#include "LostSignal.h"

namespace
{
constexpr int32 SessionDefaultMaxInventorySlotCount = 10;
constexpr int32 SessionDefaultMaxSafeSlotCount = 4;
}

void ULSSessionSubsystem::StartRaid(const TArray<FLSSessionItem>& Loadout)
{
	StartRaidInternal(Loadout, true);
}

void ULSSessionSubsystem::ClearRaidSessionState()
{
	LoadoutSnapshot.Items.Reset();
	SessionInventory.Reset();
	SessionSafeInventory.Reset();
	ConsumedItems.Reset();
	bRaidActive = false;
}

void ULSSessionSubsystem::StartRaidInternal(const TArray<FLSSessionItem>& Loadout, const bool bPersistRaidSave)
{
	LoadoutSnapshot.Items = Loadout;
	SessionInventory = Loadout;
	SessionSafeInventory.Empty();
	ConsumedItems.Empty();
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

	const int32 ToMaxSlotCount = ToArea == ELSInventorySlotArea::Inventory
		? GetMaxInventorySlotCount()
		: GetMaxSafeSlotCount();
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

	const int32 ToMaxSlotCount = ToArea == ELSInventorySlotArea::Inventory
		? GetMaxInventorySlotCount()
		: GetMaxSafeSlotCount();
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

	const int32 ToMaxSlotCount = ToArea == ELSInventorySlotArea::Inventory
		? GetMaxInventorySlotCount()
		: GetMaxSafeSlotCount();
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
	if (SlotArea == ELSInventorySlotArea::Safe && SlotIndex >= GetMaxSafeSlotCount())
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] Cannot replace safe slot because index exceeds max. Index=%d"), SlotIndex);
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
	const ULSSaveSubsystem* SaveSub = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULSSaveSubsystem>() : nullptr;
	return SaveSub ? SaveSub->GetMaxInventorySlotCount() : SessionDefaultMaxInventorySlotCount;
}

int32 ULSSessionSubsystem::GetMaxSafeSlotCount() const
{
	const ULSSaveSubsystem* SaveSub = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULSSaveSubsystem>() : nullptr;
	return SaveSub ? SaveSub->GetMaxSafeStashSlotCount() : SessionDefaultMaxSafeSlotCount;
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
