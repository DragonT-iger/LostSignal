#pragma once

#include "CoreMinimal.h"
#include "Data/LSDropSubsystem.h"
#include "Session/LSSessionSubsystem.h"

namespace LSInventorySlotUtils
{
	bool IsFilled(const FLSSessionItem& Item);
	bool IsFilled(const FLSDropResult& Item);

	FLSSessionItem MakeEmptyItem();
	FLSSessionItem ToSessionItem(const FLSDropResult& Item);
	void SetDropResultFromSessionItem(FLSDropResult& TargetSlot, const FLSSessionItem& SourceItem);
	void ClearDropResult(FLSDropResult& TargetSlot);

	int32 ResolveItemMaxStack(FName ItemRowName, const TCHAR* Context);

	// 아이템 등급을 Row Name에서 파싱한다. (예: Chip_Supply_HP / Armor_Frame_Supply / Weapon_HF_Blade_Precision)
	// Name 토큰 중 알려진 등급(Supply/Standard/Precision/Tuning/Prototype/Masterpiece)을 찾아 반환. 없으면 빈 문자열.
	FString ResolveItemGradeFromRowName(FName ItemRowName);

	void EnsureSlotIndex(TArray<FLSSessionItem>& Slots, int32 SlotIndex);
	void AddItemsToSlotArray(TArray<FLSSessionItem>& Slots, FName ItemRowName, int32 Amount);
	bool TryAddItemsToSlotArray(TArray<FLSSessionItem>& Slots, FName ItemRowName, int32 Amount, int32 MaxSlotCount, const TArray<FLSChipResolvedStat>& ChipStats, FLSSessionItem& OutRemainingItem);
	void NormalizeSlotArray(TArray<FLSSessionItem>& Slots);
	void RemoveItemsFromSlotArray(TArray<FLSSessionItem>& Slots, FName ItemRowName, int32 Amount);
	void SortAndCompactSlotArray(TArray<FLSSessionItem>& Slots);

	bool SwapSlots(TArray<FLSSessionItem>& FromSlots, int32 FromIndex, TArray<FLSSessionItem>& ToSlots, int32 ToIndex, int32 ToMaxSlotCount = INDEX_NONE);
	bool MoveSlotWithinArray(TArray<FLSSessionItem>& Slots, int32 FromIndex, int32 ToIndex);
	bool DropSlot(TArray<FLSSessionItem>& FromSlots, int32 FromIndex, TArray<FLSSessionItem>& ToSlots, int32 ToIndex, int32 ToMaxSlotCount = INDEX_NONE);
	bool DropExternalItemToSlot(FLSSessionItem& InOutExternalItem, TArray<FLSSessionItem>& ToSlots, int32 ToIndex, int32 ToMaxSlotCount = INDEX_NONE);
	bool DropResultSlot(TArray<FLSDropResult>& Slots, int32 FromIndex, int32 ToIndex);
}
