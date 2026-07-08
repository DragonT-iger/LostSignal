#pragma once

#include "CoreMinimal.h"
#include "Data/LSDropSubsystem.h"
#include "Session/LSSessionSubsystem.h"

namespace LSInventorySlotUtils
{
	// 무기/방어구 장착칸 수. 인덱스가 곧 슬롯 타입(ELSEquipmentSlot)이다.
	constexpr int32 EquipmentSlotCount = static_cast<int32>(ELSEquipmentSlot::Count);

	bool IsFilled(const FLSSessionItem& Item);
	bool IsFilled(const FLSDropResult& Item);

	// 슬롯 배열에서 MaxSlotCount 범위 안의 첫 빈(미충전) 슬롯 인덱스를 돌려준다. 없으면 INDEX_NONE.
	// 배열이 MaxSlotCount보다 짧으면 그 뒤의 아직 없는 인덱스도 빈 칸으로 간주한다.
	int32 FindFirstEmptySlotIndex(const TArray<FLSSessionItem>& Slots, int32 MaxSlotCount);

	FLSSessionItem MakeEmptyItem();
	FLSSessionItem ToSessionItem(const FLSDropResult& Item);
	void SetDropResultFromSessionItem(FLSDropResult& TargetSlot, const FLSSessionItem& SourceItem);
	void ClearDropResult(FLSDropResult& TargetSlot);

	int32 ResolveItemMaxStack(FName ItemRowName, const TCHAR* Context);

	// 아이템 등급을 Row Name에서 파싱한다. (예: Chip_Supply_HP / Armor_Frame_Supply / Weapon_HF_Blade_Precision)
	// Name 토큰 중 알려진 등급(Supply/Standard/Precision/Tuning/Prototype/Masterpiece)을 찾아 반환. 없으면 빈 문자열.
	FString ResolveItemGradeFromRowName(FName ItemRowName);

	// 칩 행 이름(Chip_{Grade}_{Func})의 기능 토큰으로 기능별 아이콘 에셋명을 반환한다.
	// 칩이 아니거나 알 수 없는 기능이면 행 이름 문자열을 그대로 반환(기존 동작 폴백).
	FString ResolveIconAssetNameFromRowName(FName ItemRowName);

	// 아이템 Row Name으로 장착 가능한 장비 슬롯 타입을 판정한다.
	// Weapon_* -> Weapon, Armor_* -> ArmorTable의 Item_Equipment(Processor/Core/Actuator/Frame).
	// 장착 불가(칩/일반 아이템/미상)면 ELSEquipmentSlot::Count 반환.
	ELSEquipmentSlot ResolveEquipmentSlotType(FName ItemRowName);

	void EnsureSlotIndex(TArray<FLSSessionItem>& Slots, int32 SlotIndex);
	void AddItemsToSlotArray(TArray<FLSSessionItem>& Slots, FName ItemRowName, int32 Amount);
	bool TryAddItemsToSlotArray(TArray<FLSSessionItem>& Slots, FName ItemRowName, int32 Amount, int32 MaxSlotCount, const TArray<FLSChipResolvedStat>& ChipStats, FLSSessionItem& OutRemainingItem);
	void NormalizeSlotArray(TArray<FLSSessionItem>& Slots);
	void RemoveItemsFromSlotArray(TArray<FLSSessionItem>& Slots, FName ItemRowName, int32 Amount);
	void SortAndCompactSlotArray(TArray<FLSSessionItem>& Slots);

	// 장비 장착/해제/교환의 공용 코어. From/To 중 정확히 하나가 장비 배열이어야 하며,
	// 장착 아이템 타입이 장비 슬롯 타입(=인덱스)과 일치할 때만 이동/스왑한다.
	// 저장(SaveSubsystem)과 레이드 세션(RaidInventoryComponent)이 같은 검증을 공유한다.
	bool MoveEquipmentSlotBetweenArrays(TArray<FLSSessionItem>& FromSlots, int32 FromIndex, bool bFromEquipment, TArray<FLSSessionItem>& ToSlots, int32 ToIndex, bool bToEquipment, int32 ToMaxSlotCount);

	bool SwapSlots(TArray<FLSSessionItem>& FromSlots, int32 FromIndex, TArray<FLSSessionItem>& ToSlots, int32 ToIndex, int32 ToMaxSlotCount = INDEX_NONE);
	bool MoveSlotWithinArray(TArray<FLSSessionItem>& Slots, int32 FromIndex, int32 ToIndex);
	bool DropSlot(TArray<FLSSessionItem>& FromSlots, int32 FromIndex, TArray<FLSSessionItem>& ToSlots, int32 ToIndex, int32 ToMaxSlotCount = INDEX_NONE);
	bool DropExternalItemToSlot(FLSSessionItem& InOutExternalItem, TArray<FLSSessionItem>& ToSlots, int32 ToIndex, int32 ToMaxSlotCount = INDEX_NONE);
	bool DropResultSlot(TArray<FLSDropResult>& Slots, int32 FromIndex, int32 ToIndex);
}
