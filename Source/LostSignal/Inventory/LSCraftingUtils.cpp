#include "Inventory/LSCraftingUtils.h"

#include "Inventory/LSInventorySlotUtils.h"

namespace
{
bool BuildCraftingMaterialTotals(const FLSCraftingRecipeRow& Recipe, TMap<FName, int32>& OutTotals)
{
	OutTotals.Reset();
	if (Recipe.ResultItemRowName.IsNone() || Recipe.GoldCost < 0)
	{
		return false;
	}

	for (const FLSCraftingMaterialCost& Material : Recipe.Materials)
	{
		if (Material.ItemRowName.IsNone() || Material.RequiredAmount <= 0
			|| Material.ItemRowName.ToString().StartsWith(TEXT("Chip_")))
		{
			return false;
		}
		OutTotals.FindOrAdd(Material.ItemRowName) += Material.RequiredAmount;
	}

	return !OutTotals.IsEmpty() || Recipe.GoldCost > 0;
}

bool HasCraftingWarehouseOverflow(const TArray<FLSSessionItem>& Slots, const int32 MaxSlotCount)
{
	for (int32 SlotIndex = FMath::Max(0, MaxSlotCount); SlotIndex < Slots.Num(); ++SlotIndex)
	{
		if (LSInventorySlotUtils::IsFilled(Slots[SlotIndex]))
		{
			return true;
		}
	}
	return false;
}

int32 CalculateCraftingResourceLimit(
	const LSCraftingUtils::FLSCraftingStorageState& State,
	const FLSCraftingRecipeRow& Recipe,
	const TMap<FName, int32>& MaterialTotals)
{
	int32 Limit = MAX_int32;
	for (const TPair<FName, int32>& Material : MaterialTotals)
	{
		Limit = FMath::Min(Limit, LSCraftingUtils::CountOwnedItem(State, Material.Key) / Material.Value);
	}
	if (Recipe.GoldCost > 0)
	{
		Limit = FMath::Min(Limit, State.Gold / Recipe.GoldCost);
	}
	return Limit == MAX_int32 ? 0 : FMath::Max(0, Limit);
}

void ConsumeCraftingMaterials(
	LSCraftingUtils::FLSCraftingStorageState& Candidate,
	const TMap<FName, int32>& MaterialTotals)
{
	for (const TPair<FName, int32>& Material : MaterialTotals)
	{
		const int32 InventoryAmount = LSCraftingUtils::CountItem(Candidate.Inventory, Material.Key);
		const int32 RemoveFromInventory = FMath::Min(InventoryAmount, Material.Value);
		LSInventorySlotUtils::RemoveItemsFromSlotArray(Candidate.Inventory, Material.Key, RemoveFromInventory);
		LSInventorySlotUtils::RemoveItemsFromSlotArray(Candidate.Warehouse, Material.Key, Material.Value - RemoveFromInventory);
	}
}

bool StoreCraftingResult(
	LSCraftingUtils::FLSCraftingStorageState& Candidate,
	const FLSCraftingRecipeRow& Recipe,
	const int32 MaxInventorySlotCount,
	const int32 MaxWarehouseSlotCount,
	const TArray<FLSChipResolvedStat>& ResultChipStats,
	bool& bOutStoredInWarehouse)
{
	FLSSessionItem RemainingItem;
	LSInventorySlotUtils::TryAddItemsToSlotArray(
		Candidate.Inventory, Recipe.ResultItemRowName, 1, MaxInventorySlotCount, ResultChipStats, RemainingItem);
	if (!LSInventorySlotUtils::IsFilled(RemainingItem))
	{
		return true;
	}
	if (HasCraftingWarehouseOverflow(Candidate.Warehouse, MaxWarehouseSlotCount))
	{
		return false;
	}

	RemainingItem = LSInventorySlotUtils::MakeEmptyItem();
	LSInventorySlotUtils::TryAddItemsToSlotArray(
		Candidate.Warehouse, Recipe.ResultItemRowName, 1, MaxWarehouseSlotCount, ResultChipStats, RemainingItem);
	bOutStoredInWarehouse = !LSInventorySlotUtils::IsFilled(RemainingItem);
	return bOutStoredInWarehouse;
}
}

namespace LSCraftingUtils
{
bool IsRecipeValid(const FLSCraftingRecipeRow& Recipe)
{
	TMap<FName, int32> MaterialTotals;
	return BuildCraftingMaterialTotals(Recipe, MaterialTotals);
}

bool ResolveCategory(const FName ItemRowName, ELSCraftingCategory& OutCategory)
{
	const FString RowNameString = ItemRowName.ToString();
	if (RowNameString.StartsWith(TEXT("Weapon_")))
	{
		OutCategory = ELSCraftingCategory::Weapon;
		return true;
	}
	if (RowNameString.StartsWith(TEXT("Armor_")))
	{
		OutCategory = ELSCraftingCategory::Armor;
		return true;
	}
	if (RowNameString.StartsWith(TEXT("Chip_")))
	{
		OutCategory = ELSCraftingCategory::Chip;
		return true;
	}
	if (RowNameString.StartsWith(TEXT("Item_")))
	{
		const LSInventorySlotUtils::FLSItemTradeInfo Info = LSInventorySlotUtils::ResolveItemTradeInfo(ItemRowName);
		if (Info.bValid && Info.ItemType >= 4 && Info.ItemType <= 9)
		{
			OutCategory = ELSCraftingCategory::Consumable;
			return true;
		}
	}
	return false;
}

int32 CountItem(const TArray<FLSSessionItem>& Slots, const FName ItemRowName)
{
	int32 Count = 0;
	for (const FLSSessionItem& Slot : Slots)
	{
		if (Slot.ItemRowName == ItemRowName && Slot.Amount > 0)
		{
			Count += Slot.Amount;
		}
	}
	return Count;
}

int32 CountOwnedItem(const FLSCraftingStorageState& State, const FName ItemRowName)
{
	return CountItem(State.Inventory, ItemRowName) + CountItem(State.Warehouse, ItemRowName);
}

bool TryCraftOne(
	FLSCraftingStorageState& InOutState,
	const FLSCraftingRecipeRow& Recipe,
	const int32 MaxInventorySlotCount,
	const int32 MaxWarehouseSlotCount,
	const TArray<FLSChipResolvedStat>& ResultChipStats,
	bool& bOutStoredInWarehouse)
{
	bOutStoredInWarehouse = false;
	TMap<FName, int32> MaterialTotals;
	if (!BuildCraftingMaterialTotals(Recipe, MaterialTotals) || InOutState.Gold < Recipe.GoldCost)
	{
		return false;
	}

	for (const TPair<FName, int32>& Material : MaterialTotals)
	{
		if (CountOwnedItem(InOutState, Material.Key) < Material.Value)
		{
			return false;
		}
	}

	FLSCraftingStorageState Candidate = InOutState;
	ConsumeCraftingMaterials(Candidate, MaterialTotals);
	Candidate.Gold -= Recipe.GoldCost;
	if (!StoreCraftingResult(
		Candidate,
		Recipe,
		MaxInventorySlotCount,
		MaxWarehouseSlotCount,
		ResultChipStats,
		bOutStoredInWarehouse))
	{
		return false;
	}

	InOutState = MoveTemp(Candidate);
	return true;
}

int32 CalculateCraftableCount(
	const FLSCraftingStorageState& State,
	const FLSCraftingRecipeRow& Recipe,
	const int32 MaxInventorySlotCount,
	const int32 MaxWarehouseSlotCount)
{
	TMap<FName, int32> MaterialTotals;
	if (!BuildCraftingMaterialTotals(Recipe, MaterialTotals))
	{
		return 0;
	}

	const int32 ResourceLimit = CalculateCraftingResourceLimit(State, Recipe, MaterialTotals);
	FLSCraftingStorageState Candidate = State;
	TArray<FLSChipResolvedStat> PreviewChipStats;
	if (Recipe.ResultItemRowName.ToString().StartsWith(TEXT("Chip_")))
	{
		FLSChipResolvedStat InstanceMarker;
		InstanceMarker.StatKey = TEXT("CraftingPreview");
		PreviewChipStats.Add(InstanceMarker);
	}

	int32 CraftableCount = 0;
	for (; CraftableCount < ResourceLimit; ++CraftableCount)
	{
		bool bStoredInWarehouse = false;
		if (!TryCraftOne(Candidate, Recipe, MaxInventorySlotCount, MaxWarehouseSlotCount, PreviewChipStats, bStoredInWarehouse))
		{
			break;
		}
	}
	return CraftableCount;
}
}
