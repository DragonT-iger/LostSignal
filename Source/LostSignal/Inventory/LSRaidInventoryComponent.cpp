#include "Inventory/LSRaidInventoryComponent.h"

#include "Inventory/LSInventorySlotUtils.h"
#include "LostSignal.h"
#include "Session/LSSaveSubsystem.h"

namespace
{
constexpr int32 DefaultMaxRaidInventorySlotCount = 10;
constexpr int32 DefaultMaxRaidSafeSlotCount = 4;
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
	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const ULSSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
	return SaveSubsystem ? SaveSubsystem->GetMaxInventorySlotCount() : DefaultMaxRaidInventorySlotCount;
}

int32 ULSRaidInventoryComponent::GetMaxSafeSlotCount() const
{
	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	const ULSSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
	return SaveSubsystem ? SaveSubsystem->GetMaxSafeStashSlotCount() : DefaultMaxRaidSafeSlotCount;
}

bool ULSRaidInventoryComponent::TryAddSessionItem(const FName ItemRowName, const int32 Amount, const TArray<FLSChipResolvedStat>& ChipStats, FLSSessionItem& OutRemainingItem)
{
	return LSInventorySlotUtils::TryAddItemsToSlotArray(SessionInventory, ItemRowName, Amount, GetMaxInventorySlotCount(), ChipStats, OutRemainingItem);
}

void ULSRaidInventoryComponent::SortSessionInventory()
{
	LSInventorySlotUtils::SortAndCompactSlotArray(SessionInventory);
}

bool ULSRaidInventoryComponent::DropSessionSlot(const ELSInventorySlotArea FromArea, const int32 FromIndex, const ELSInventorySlotArea ToArea, const int32 ToIndex)
{
	if (FromArea == ELSInventorySlotArea::Warehouse || ToArea == ELSInventorySlotArea::Warehouse)
	{
		return false;
	}

	TArray<FLSSessionItem>* FromSlots = FromArea == ELSInventorySlotArea::Safe ? &SessionSafeInventory : &SessionInventory;
	TArray<FLSSessionItem>* ToSlots = ToArea == ELSInventorySlotArea::Safe ? &SessionSafeInventory : &SessionInventory;
	const int32 ToMaxSlotCount = ToArea == ELSInventorySlotArea::Inventory
		? GetMaxInventorySlotCount()
		: GetMaxSafeSlotCount();
	return LSInventorySlotUtils::DropSlot(*FromSlots, FromIndex, *ToSlots, ToIndex, ToMaxSlotCount);
}

bool ULSRaidInventoryComponent::DropExternalItemToSessionSlot(FLSSessionItem& InOutExternalItem, const ELSInventorySlotArea ToArea, const int32 ToIndex)
{
	if (ToArea == ELSInventorySlotArea::Warehouse)
	{
		return false;
	}

	TArray<FLSSessionItem>* ToSlots = ToArea == ELSInventorySlotArea::Safe ? &SessionSafeInventory : &SessionInventory;
	const int32 ToMaxSlotCount = ToArea == ELSInventorySlotArea::Inventory
		? GetMaxInventorySlotCount()
		: GetMaxSafeSlotCount();
	return LSInventorySlotUtils::DropExternalItemToSlot(InOutExternalItem, *ToSlots, ToIndex, ToMaxSlotCount);
}

bool ULSRaidInventoryComponent::GetSessionSlotItem(const ELSInventorySlotArea SlotArea, const int32 SlotIndex, FLSSessionItem& OutItem) const
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

bool ULSRaidInventoryComponent::ClearSessionSlot(const ELSInventorySlotArea SlotArea, const int32 SlotIndex)
{
	if (SlotArea == ELSInventorySlotArea::Warehouse)
	{
		return false;
	}

	TArray<FLSSessionItem>& Slots = SlotArea == ELSInventorySlotArea::Safe ? SessionSafeInventory : SessionInventory;
	if (!Slots.IsValidIndex(SlotIndex) || !LSInventorySlotUtils::IsFilled(Slots[SlotIndex]))
	{
		return false;
	}

	Slots[SlotIndex] = LSInventorySlotUtils::MakeEmptyItem();
	return true;
}

bool ULSRaidInventoryComponent::ReplaceSessionSlotItem(const ELSInventorySlotArea SlotArea, const int32 SlotIndex, const FLSSessionItem& NewItem, FLSSessionItem& OutPreviousItem)
{
	OutPreviousItem = FLSSessionItem();
	if (SlotArea == ELSInventorySlotArea::Warehouse)
	{
		return false;
	}

	if (SlotIndex < 0)
	{
		return false;
	}

	if (SlotArea == ELSInventorySlotArea::Inventory && SlotIndex >= GetMaxInventorySlotCount())
	{
		return false;
	}
	if (SlotArea == ELSInventorySlotArea::Safe && SlotIndex >= GetMaxSafeSlotCount())
	{
		return false;
	}

	TArray<FLSSessionItem>& Slots = SlotArea == ELSInventorySlotArea::Safe ? SessionSafeInventory : SessionInventory;
	LSInventorySlotUtils::EnsureSlotIndex(Slots, SlotIndex);
	OutPreviousItem = Slots[SlotIndex];
	Slots[SlotIndex] = NewItem;
	return true;
}
