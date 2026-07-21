#include "Data/LSCraftingRecipeRow.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Inventory/LSCraftingUtils.h"
#include "Misc/AutomationTest.h"

namespace
{
FLSSessionItem MakeCraftingTestItem(const FName RowName, const int32 Amount)
{
	FLSSessionItem Item;
	Item.ItemRowName = RowName;
	Item.Amount = Amount;
	return Item;
}

FLSCraftingRecipeRow MakeCraftingTestRecipe(
	const FName ResultRowName,
	const FName MaterialRowName,
	const int32 RequiredAmount,
	const int32 GoldCost)
{
	FLSCraftingRecipeRow Recipe;
	Recipe.ResultItemRowName = ResultRowName;
	Recipe.GoldCost = GoldCost;
	if (!MaterialRowName.IsNone())
	{
		FLSCraftingMaterialCost& Material = Recipe.Materials.AddDefaulted_GetRef();
		Material.ItemRowName = MaterialRowName;
		Material.RequiredAmount = RequiredAmount;
	}
	return Recipe;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLSCraftingTransactionTest,
	"LostSignal.Crafting.Transaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLSCraftingTransactionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const FName MaterialRow(TEXT("Item_ScrapMetalFragment_Supply"));
	const FName ResultRow(TEXT("Armor_Core_Tuning"));
	const FName OccupiedRow(TEXT("Armor_Core_Prototype"));

	LSCraftingUtils::FLSCraftingStorageState CombinedState;
	CombinedState.Inventory = { MakeCraftingTestItem(MaterialRow, 2), MakeCraftingTestItem(OccupiedRow, 1) };
	CombinedState.Warehouse = { MakeCraftingTestItem(MaterialRow, 1) };
	CombinedState.Gold = 100;
	const FLSCraftingRecipeRow CombinedRecipe = MakeCraftingTestRecipe(ResultRow, MaterialRow, 3, 10);
	bool bStoredInWarehouse = false;
	TestTrue(TEXT("Inventory and warehouse materials are combined"), LSCraftingUtils::TryCraftOne(
		CombinedState, CombinedRecipe, 2, 2, TArray<FLSChipResolvedStat>(), bStoredInWarehouse));
	TestFalse(TEXT("Inventory space freed by materials is used first"), bStoredInWarehouse);
	TestEqual(TEXT("Inventory materials consumed first"), LSCraftingUtils::CountItem(CombinedState.Inventory, MaterialRow), 0);
	TestEqual(TEXT("Warehouse remainder consumed"), LSCraftingUtils::CountItem(CombinedState.Warehouse, MaterialRow), 0);
	TestEqual(TEXT("Result stored in inventory"), LSCraftingUtils::CountItem(CombinedState.Inventory, ResultRow), 1);
	TestEqual(TEXT("Gold spent once"), CombinedState.Gold, 90);

	LSCraftingUtils::FLSCraftingStorageState FallbackState;
	FallbackState.Inventory = { MakeCraftingTestItem(OccupiedRow, 1) };
	FallbackState.Gold = 10;
	const FLSCraftingRecipeRow GoldOnlyRecipe = MakeCraftingTestRecipe(ResultRow, NAME_None, 0, 10);
	TestTrue(TEXT("Warehouse fallback succeeds"), LSCraftingUtils::TryCraftOne(
		FallbackState, GoldOnlyRecipe, 1, 1, TArray<FLSChipResolvedStat>(), bStoredInWarehouse));
	TestTrue(TEXT("Warehouse fallback reported"), bStoredInWarehouse);
	TestEqual(TEXT("Result stored in warehouse"), LSCraftingUtils::CountItem(FallbackState.Warehouse, ResultRow), 1);

	LSCraftingUtils::FLSCraftingStorageState FullState;
	FullState.Inventory = { MakeCraftingTestItem(OccupiedRow, 1) };
	FullState.Warehouse = { MakeCraftingTestItem(OccupiedRow, 1) };
	FullState.Gold = 10;
	TestFalse(TEXT("Craft fails when both destinations are full"), LSCraftingUtils::TryCraftOne(
		FullState, GoldOnlyRecipe, 1, 1, TArray<FLSChipResolvedStat>(), bStoredInWarehouse));
	TestEqual(TEXT("Failed craft preserves gold"), FullState.Gold, 10);
	TestEqual(TEXT("Failed craft preserves inventory"), LSCraftingUtils::CountItem(FullState.Inventory, OccupiedRow), 1);
	TestEqual(TEXT("Failed craft preserves warehouse"), LSCraftingUtils::CountItem(FullState.Warehouse, OccupiedRow), 1);

	LSCraftingUtils::FLSCraftingStorageState CountState;
	CountState.Inventory = { MakeCraftingTestItem(MaterialRow, 2) };
	CountState.Gold = 100;
	const FLSCraftingRecipeRow CountRecipe = MakeCraftingTestRecipe(ResultRow, MaterialRow, 1, 10);
	TestEqual(TEXT("Craftable count includes freed inventory space and fallback"),
		LSCraftingUtils::CalculateCraftableCount(CountState, CountRecipe, 1, 1), 2);

	FLSCraftingRecipeRow DuplicateRecipe = MakeCraftingTestRecipe(ResultRow, MaterialRow, 2, 0);
	FLSCraftingMaterialCost& DuplicateMaterial = DuplicateRecipe.Materials.AddDefaulted_GetRef();
	DuplicateMaterial.ItemRowName = MaterialRow;
	DuplicateMaterial.RequiredAmount = 1;
	LSCraftingUtils::FLSCraftingStorageState DuplicateState;
	DuplicateState.Inventory = { MakeCraftingTestItem(MaterialRow, 2) };
	TestFalse(TEXT("Duplicate material rows are summed before validation"), LSCraftingUtils::TryCraftOne(
		DuplicateState, DuplicateRecipe, 2, 2, TArray<FLSChipResolvedStat>(), bStoredInWarehouse));

	FLSCraftingRecipeRow ManyMaterialsRecipe;
	ManyMaterialsRecipe.ResultItemRowName = ResultRow;
	for (int32 Index = 0; Index < 5; ++Index)
	{
		FLSCraftingMaterialCost& Material = ManyMaterialsRecipe.Materials.AddDefaulted_GetRef();
		Material.ItemRowName = FName(*FString::Printf(TEXT("Item_CraftingMaterial_%d"), Index));
		Material.RequiredAmount = 1;
	}
	TestTrue(TEXT("Recipe material count is not capped"), LSCraftingUtils::IsRecipeValid(ManyMaterialsRecipe));

	ELSCraftingCategory Category = ELSCraftingCategory::All;
	TestTrue(TEXT("Weapon category"), LSCraftingUtils::ResolveCategory(TEXT("Weapon_Test"), Category));
	TestEqual(TEXT("Weapon category value"), static_cast<uint8>(Category), static_cast<uint8>(ELSCraftingCategory::Weapon));
	TestTrue(TEXT("Armor category"), LSCraftingUtils::ResolveCategory(TEXT("Armor_Test"), Category));
	TestEqual(TEXT("Armor category value"), static_cast<uint8>(Category), static_cast<uint8>(ELSCraftingCategory::Armor));
	TestTrue(TEXT("Chip category"), LSCraftingUtils::ResolveCategory(TEXT("Chip_Test"), Category));
	TestEqual(TEXT("Chip category value"), static_cast<uint8>(Category), static_cast<uint8>(ELSCraftingCategory::Chip));

	return true;
}

#endif
