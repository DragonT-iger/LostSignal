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

void ULSRaidInventoryComponent::StartRaidInventory(const TArray<FLSSessionItem>& Loadout, const TArray<FLSSessionItem>& SafeItems, const TArray<FLSSessionItem>& EquipmentItems)
{
	LoadoutSnapshot.Items = Loadout;
	SessionInventory = Loadout;
	SessionSafeInventory = SafeItems;
	// 장비는 인덱스=슬롯 타입이므로 정렬/압축 없이 5칸 패딩만 한다.
	SessionEquipmentSlots = EquipmentItems;
	SessionEquipmentSlots.SetNum(LSInventorySlotUtils::EquipmentSlotCount);
	ConsumedItems.Empty();
	bRaidActive = true;
}

void ULSRaidInventoryComponent::MirrorRaidInventoryState(const TArray<FLSSessionItem>& InventoryItems, const TArray<FLSSessionItem>& SafeItems, const TArray<FLSSessionItem>& EquipmentItems)
{
	SessionInventory = InventoryItems;
	SessionSafeInventory = SafeItems;
	SessionEquipmentSlots = EquipmentItems;
	SessionEquipmentSlots.SetNum(LSInventorySlotUtils::EquipmentSlotCount);
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
	if (FromArea == ELSInventorySlotArea::Safe && FromIndex >= GetMaxSafeSlotCount())
	{
		return false;
	}

	TArray<FLSSessionItem>* FromSlots = ResolveSessionSlots(FromArea);
	TArray<FLSSessionItem>* ToSlots = ResolveSessionSlots(ToArea);
	if (!FromSlots || !ToSlots)
	{
		return false;
	}

	const int32 ToMaxSlotCount = ResolveMaxSlotCount(ToArea);

	// 장착칸이 원본/대상에 끼면 타입 검증이 필요한 장비 이동 코어를 쓴다(로비 저장 경로와 동일 규칙).
	const bool bFromEquipment = FromArea == ELSInventorySlotArea::Equipment;
	const bool bToEquipment = ToArea == ELSInventorySlotArea::Equipment;
	if (bFromEquipment || bToEquipment)
	{
		return LSInventorySlotUtils::MoveEquipmentSlotBetweenArrays(*FromSlots, FromIndex, bFromEquipment, *ToSlots, ToIndex, bToEquipment, ToMaxSlotCount);
	}

	return LSInventorySlotUtils::DropSlot(*FromSlots, FromIndex, *ToSlots, ToIndex, ToMaxSlotCount);
}

bool ULSRaidInventoryComponent::DropExternalItemToSessionSlot(FLSSessionItem& InOutExternalItem, const ELSInventorySlotArea ToArea, const int32 ToIndex)
{
	TArray<FLSSessionItem>* ToSlots = ResolveSessionSlots(ToArea);
	if (!ToSlots)
	{
		return false;
	}

	// 외부 아이템(룻박스 등)을 장착칸에 직접 놓는 경우: 빈 칸 + 타입 일치일 때만 허용한다.
	// 채워진 장착칸과의 스택 병합/스왑은 외부 아이템의 되돌림 경로가 없어 지원하지 않는다.
	if (ToArea == ELSInventorySlotArea::Equipment)
	{
		if (ToSlots->IsValidIndex(ToIndex) && LSInventorySlotUtils::IsFilled((*ToSlots)[ToIndex]))
		{
			return false;
		}
		if (LSInventorySlotUtils::ResolveEquipmentSlotType(InOutExternalItem.ItemRowName) != static_cast<ELSEquipmentSlot>(ToIndex))
		{
			return false;
		}
	}

	return LSInventorySlotUtils::DropExternalItemToSlot(InOutExternalItem, *ToSlots, ToIndex, ResolveMaxSlotCount(ToArea));
}

bool ULSRaidInventoryComponent::GetSessionSlotItem(const ELSInventorySlotArea SlotArea, const int32 SlotIndex, FLSSessionItem& OutItem) const
{
	const TArray<FLSSessionItem>* Slots = ResolveSessionSlots(SlotArea);
	if (!Slots || !Slots->IsValidIndex(SlotIndex) || !LSInventorySlotUtils::IsFilled((*Slots)[SlotIndex]))
	{
		return false;
	}

	OutItem = (*Slots)[SlotIndex];
	return true;
}

bool ULSRaidInventoryComponent::ClearSessionSlot(const ELSInventorySlotArea SlotArea, const int32 SlotIndex)
{
	TArray<FLSSessionItem>* Slots = ResolveSessionSlots(SlotArea);
	if (!Slots || !Slots->IsValidIndex(SlotIndex) || !LSInventorySlotUtils::IsFilled((*Slots)[SlotIndex]))
	{
		return false;
	}

	(*Slots)[SlotIndex] = LSInventorySlotUtils::MakeEmptyItem();
	return true;
}

bool ULSRaidInventoryComponent::ReplaceSessionSlotItem(const ELSInventorySlotArea SlotArea, const int32 SlotIndex, const FLSSessionItem& NewItem, FLSSessionItem& OutPreviousItem)
{
	OutPreviousItem = FLSSessionItem();
	if (SlotIndex < 0 || SlotIndex >= ResolveMaxSlotCount(SlotArea))
	{
		return false;
	}

	TArray<FLSSessionItem>* Slots = ResolveSessionSlots(SlotArea);
	if (!Slots)
	{
		return false;
	}

	// 장착칸 교체는 새 아이템이 비어 있거나(비우기) 슬롯 타입과 일치할 때만 허용한다.
	if (SlotArea == ELSInventorySlotArea::Equipment
		&& LSInventorySlotUtils::IsFilled(NewItem)
		&& LSInventorySlotUtils::ResolveEquipmentSlotType(NewItem.ItemRowName) != static_cast<ELSEquipmentSlot>(SlotIndex))
	{
		return false;
	}

	LSInventorySlotUtils::EnsureSlotIndex(*Slots, SlotIndex);
	OutPreviousItem = (*Slots)[SlotIndex];
	(*Slots)[SlotIndex] = NewItem;
	return true;
}

TArray<FLSSessionItem>* ULSRaidInventoryComponent::ResolveSessionSlots(const ELSInventorySlotArea SlotArea)
{
	switch (SlotArea)
	{
	case ELSInventorySlotArea::Inventory:
		return &SessionInventory;
	case ELSInventorySlotArea::Safe:
		return &SessionSafeInventory;
	case ELSInventorySlotArea::Equipment:
		return &SessionEquipmentSlots;
	default:
		// Warehouse 등은 레이드 세션이 다루지 않는 영역이다.
		return nullptr;
	}
}

const TArray<FLSSessionItem>* ULSRaidInventoryComponent::ResolveSessionSlots(const ELSInventorySlotArea SlotArea) const
{
	return const_cast<ULSRaidInventoryComponent*>(this)->ResolveSessionSlots(SlotArea);
}

int32 ULSRaidInventoryComponent::ResolveMaxSlotCount(const ELSInventorySlotArea SlotArea) const
{
	switch (SlotArea)
	{
	case ELSInventorySlotArea::Inventory:
		return GetMaxInventorySlotCount();
	case ELSInventorySlotArea::Safe:
		return GetMaxSafeSlotCount();
	case ELSInventorySlotArea::Equipment:
		return LSInventorySlotUtils::EquipmentSlotCount;
	default:
		return 0;
	}
}
