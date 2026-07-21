#pragma once

#include "CoreMinimal.h"
#include "Data/LSCraftingRecipeRow.h"
#include "Session/LSSessionSubsystem.h"

namespace LSCraftingUtils
{
	struct FLSCraftingStorageState
	{
		TArray<FLSSessionItem> Inventory;
		TArray<FLSSessionItem> Warehouse;
		int32 Gold = 0;
	};

	LOSTSIGNAL_API bool IsRecipeValid(const FLSCraftingRecipeRow& Recipe);
	LOSTSIGNAL_API bool ResolveCategory(FName ItemRowName, ELSCraftingCategory& OutCategory);
	LOSTSIGNAL_API int32 CountItem(const TArray<FLSSessionItem>& Slots, FName ItemRowName);
	LOSTSIGNAL_API int32 CountOwnedItem(const FLSCraftingStorageState& State, FName ItemRowName);

	// 복사된 상태에 제작 1회를 적용한다. 실패하면 InOutState는 전혀 바뀌지 않는다.
	LOSTSIGNAL_API bool TryCraftOne(
		FLSCraftingStorageState& InOutState,
		const FLSCraftingRecipeRow& Recipe,
		int32 MaxInventorySlotCount,
		int32 MaxWarehouseSlotCount,
		const TArray<FLSChipResolvedStat>& ResultChipStats,
		bool& bOutStoredInWarehouse);

	// 재료/골드/결과물 공간을 모두 반영해 현재 상태에서 연속 제작 가능한 횟수를 계산한다.
	LOSTSIGNAL_API int32 CalculateCraftableCount(
		const FLSCraftingStorageState& State,
		const FLSCraftingRecipeRow& Recipe,
		int32 MaxInventorySlotCount,
		int32 MaxWarehouseSlotCount);
}
