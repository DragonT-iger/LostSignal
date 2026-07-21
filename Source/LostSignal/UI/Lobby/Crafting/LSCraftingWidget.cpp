#include "UI/Lobby/Crafting/LSCraftingWidget.h"

#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/WrapBox.h"
#include "Data/LSDropSettings.h"
#include "Engine/DataTable.h"
#include "Inventory/LSCraftingUtils.h"
#include "Inventory/LSInventorySlotUtils.h"
#include "LostSignal.h"
#include "Session/LSSaveSubsystem.h"
#include "UI/Lobby/Crafting/LSCraftingMaterialSlotWidget.h"
#include "UI/Lobby/Crafting/LSCraftingRowWidget.h"
#include "UI/Lobby/Crafting/LSCraftingTabWidget.h"

#define LOCTEXT_NAMESPACE "LSCrafting"

void ULSCraftingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	const TCHAR* WidgetNames[] = {
		TEXT("AllTab"), TEXT("WeaponTab"), TEXT("ArmorTab"), TEXT("ChipTab"), TEXT("ConsumableTab"),
		TEXT("RecipeList"), TEXT("DetailPanel"), TEXT("MaterialList"),
		TEXT("GoldCostText"), TEXT("CraftableCountText"), TEXT("CraftButton") };
	const UWidget* Widgets[] = {
		AllTab, WeaponTab, ArmorTab, ChipTab, ConsumableTab,
		RecipeList, DetailPanel, MaterialList,
		GoldCostText, CraftableCountText, CraftButton };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Widgets); ++Index)
	{
		if (!Widgets[Index])
		{
			UE_LOG(LogLS, Warning, TEXT("[Crafting] %s is not bound on %s."), WidgetNames[Index], *GetNameSafe(this));
		}
	}
	if (!CraftingRowClass)
	{
		UE_LOG(LogLS, Warning, TEXT("[Crafting] CraftingRowClass is not set on %s."), *GetNameSafe(this));
	}
	if (!MaterialSlotClass)
	{
		UE_LOG(LogLS, Warning, TEXT("[Crafting] MaterialSlotClass is not set on %s."), *GetNameSafe(this));
	}
	if (MaterialList)
	{
		MaterialList->ClearChildren();
	}

	ULSCraftingTabWidget* CategoryTabs[] = { AllTab, WeaponTab, ArmorTab, ChipTab, ConsumableTab };
	const FText CategoryLabels[] = {
		LOCTEXT("AllCategory", "ALL"), LOCTEXT("WeaponCategory", "무기"), LOCTEXT("ArmorCategory", "방어구"),
		LOCTEXT("ChipCategory", "칩"), LOCTEXT("ConsumableCategory", "소모품") };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(CategoryTabs); ++Index)
	{
		if (CategoryTabs[Index])
		{
			CategoryTabs[Index]->SetLabel(CategoryLabels[Index]);
			CategoryTabs[Index]->OnClicked.AddDynamic(this, &ULSCraftingWidget::HandleTabClicked);
		}
	}
	if (CraftButton)
	{
		CraftButton->OnClicked.AddDynamic(this, &ULSCraftingWidget::HandleCraftClicked);
	}

	OpenCrafting();
}

void ULSCraftingWidget::NativeDestruct()
{
	ULSCraftingTabWidget* CategoryTabs[] = { AllTab, WeaponTab, ArmorTab, ChipTab, ConsumableTab };
	for (ULSCraftingTabWidget* CategoryTab : CategoryTabs)
	{
		if (CategoryTab)
		{
			CategoryTab->OnClicked.RemoveAll(this);
		}
	}
	for (ULSCraftingRowWidget* RecipeRow : ActiveRows)
	{
		if (RecipeRow)
		{
			RecipeRow->OnClicked.RemoveAll(this);
		}
	}
	if (CraftButton)
	{
		CraftButton->OnClicked.RemoveDynamic(this, &ULSCraftingWidget::HandleCraftClicked);
	}
	Super::NativeDestruct();
}

void ULSCraftingWidget::OpenCrafting()
{
	LoadRecipes();
	SetCategory(ELSCraftingCategory::All);
}

void ULSCraftingWidget::HandleTabClicked(ULSCraftingTabWidget* ClickedTab)
{
	if (ClickedTab == AllTab) { SetCategory(ELSCraftingCategory::All); }
	else if (ClickedTab == WeaponTab) { SetCategory(ELSCraftingCategory::Weapon); }
	else if (ClickedTab == ArmorTab) { SetCategory(ELSCraftingCategory::Armor); }
	else if (ClickedTab == ChipTab) { SetCategory(ELSCraftingCategory::Chip); }
	else if (ClickedTab == ConsumableTab) { SetCategory(ELSCraftingCategory::Consumable); }
}

void ULSCraftingWidget::HandleRecipeClicked(ULSCraftingRowWidget* ClickedRow)
{
	if (ClickedRow)
	{
		SelectRecipe(ClickedRow->GetRecipeRowName());
	}
}

void ULSCraftingWidget::HandleCraftClicked()
{
	const FRecipeEntry* SelectedRecipe = FindRecipe(SelectedRecipeRowName);
	ULSSaveSubsystem* SaveSubsystem = GetSaveSubsystem();
	if (!SelectedRecipe || !SaveSubsystem)
	{
		return;
	}

	bool bStoredInWarehouse = false;
	if (!SaveSubsystem->TryCraft(SelectedRecipe->Recipe, bStoredInWarehouse))
	{
		UE_LOG(LogLS, Log, TEXT("[Crafting] Craft request was rejected for %s."), *SelectedRecipeRowName.ToString());
		RefreshDetail();
		return;
	}

	RefreshRecipeRows();
	RefreshDetail();
}

void ULSCraftingWidget::LoadRecipes()
{
	Recipes.Reset();
	const ULSDropSettings* Settings = GetDefault<ULSDropSettings>();
	UDataTable* RecipeTable = Settings ? Settings->CraftingRecipeTable.LoadSynchronous() : nullptr;
	if (!RecipeTable)
	{
		UE_LOG(LogLS, Warning, TEXT("[Crafting] CraftingRecipeTable is not set on %s."), *GetNameSafe(this));
		return;
	}

	for (const FName RowName : RecipeTable->GetRowNames())
	{
		const FLSCraftingRecipeRow* Recipe = RecipeTable->FindRow<FLSCraftingRecipeRow>(RowName, TEXT("LoadCraftingRecipes"));
		ELSCraftingCategory IgnoredCategory = ELSCraftingCategory::All;
		if (!Recipe || !LSCraftingUtils::IsRecipeValid(*Recipe)
			|| !LSInventorySlotUtils::ResolveItemTradeInfo(Recipe->ResultItemRowName).bValid
			|| !LSCraftingUtils::ResolveCategory(Recipe->ResultItemRowName, IgnoredCategory))
		{
			UE_LOG(LogLS, Warning, TEXT("[Crafting] Invalid recipe row skipped: %s"), *RowName.ToString());
			continue;
		}

		bool bMaterialsValid = true;
		for (const FLSCraftingMaterialCost& Material : Recipe->Materials)
		{
			if (!LSInventorySlotUtils::ResolveItemTradeInfo(Material.ItemRowName).bValid)
			{
				bMaterialsValid = false;
				break;
			}
		}
		if (!bMaterialsValid)
		{
			UE_LOG(LogLS, Warning, TEXT("[Crafting] Recipe has a missing material row: %s"), *RowName.ToString());
			continue;
		}

		FRecipeEntry& Entry = Recipes.AddDefaulted_GetRef();
		Entry.RowName = RowName;
		Entry.Recipe = *Recipe;
	}

	Recipes.Sort([](const FRecipeEntry& Left, const FRecipeEntry& Right)
	{
		return Left.Recipe.SortOrder == Right.Recipe.SortOrder
			? Left.RowName.LexicalLess(Right.RowName)
			: Left.Recipe.SortOrder < Right.Recipe.SortOrder;
	});
}

void ULSCraftingWidget::SetCategory(const ELSCraftingCategory NewCategory)
{
	CurrentCategory = NewCategory;
	ULSCraftingTabWidget* CategoryTabs[] = { AllTab, WeaponTab, ArmorTab, ChipTab, ConsumableTab };
	const ELSCraftingCategory Categories[] = {
		ELSCraftingCategory::All, ELSCraftingCategory::Weapon, ELSCraftingCategory::Armor,
		ELSCraftingCategory::Chip, ELSCraftingCategory::Consumable };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(CategoryTabs); ++Index)
	{
		if (CategoryTabs[Index])
		{
			CategoryTabs[Index]->SetSelected(Categories[Index] == CurrentCategory);
		}
	}
	RebuildRecipeList();
}

void ULSCraftingWidget::RebuildRecipeList()
{
	ClearRecipeRows();
	if (!RecipeList || !CraftingRowClass)
	{
		ClearSelection();
		return;
	}

	ULSSaveSubsystem* SaveSubsystem = GetSaveSubsystem();
	for (const FRecipeEntry& Entry : Recipes)
	{
		ELSCraftingCategory RecipeCategory = ELSCraftingCategory::All;
		if (!LSCraftingUtils::ResolveCategory(Entry.Recipe.ResultItemRowName, RecipeCategory)
			|| (CurrentCategory != ELSCraftingCategory::All && CurrentCategory != RecipeCategory))
		{
			continue;
		}

		ULSCraftingRowWidget* NewRow = CreateRecipeRow(Entry, SaveSubsystem);
		if (!NewRow)
		{
			continue;
		}
		RecipeList->AddChild(NewRow);
		ActiveRows.Add(NewRow);
	}

	if (!ActiveRows.IsEmpty())
	{
		SelectRecipe(ActiveRows[0]->GetRecipeRowName());
	}
	else
	{
		ClearSelection();
	}
}

void ULSCraftingWidget::ClearRecipeRows()
{
	for (ULSCraftingRowWidget* RecipeRow : ActiveRows)
	{
		if (RecipeRow)
		{
			RecipeRow->OnClicked.RemoveAll(this);
		}
	}
	ActiveRows.Reset();
	if (RecipeList)
	{
		RecipeList->ClearChildren();
	}
}

ULSCraftingRowWidget* ULSCraftingWidget::CreateRecipeRow(
	const FRecipeEntry& Entry,
	ULSSaveSubsystem* SaveSubsystem)
{
	ULSCraftingRowWidget* NewRow = CreateWidget<ULSCraftingRowWidget>(this, CraftingRowClass);
	if (!NewRow)
	{
		UE_LOG(LogLS, Warning, TEXT("[Crafting] Failed to create recipe row on %s."), *GetNameSafe(this));
		return nullptr;
	}
	const LSInventorySlotUtils::FLSItemTradeInfo Info =
		LSInventorySlotUtils::ResolveItemTradeInfo(Entry.Recipe.ResultItemRowName);
	const int32 OwnedAmount = SaveSubsystem
		? SaveSubsystem->GetCraftingOwnedItemCount(Entry.Recipe.ResultItemRowName)
		: 0;
	NewRow->SetRecipe(Entry.RowName, Entry.Recipe.ResultItemRowName, Info.Name, OwnedAmount);
	NewRow->OnClicked.AddDynamic(this, &ULSCraftingWidget::HandleRecipeClicked);
	return NewRow;
}

void ULSCraftingWidget::SelectRecipe(const FName RecipeRowName)
{
	SelectedRecipeRowName = RecipeRowName;
	for (ULSCraftingRowWidget* RecipeRow : ActiveRows)
	{
		if (RecipeRow)
		{
			RecipeRow->SetSelected(RecipeRow->GetRecipeRowName() == SelectedRecipeRowName);
		}
	}
	RefreshDetail();
}

void ULSCraftingWidget::ClearSelection()
{
	SelectedRecipeRowName = NAME_None;
	if (DetailPanel)
	{
		DetailPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (CraftButton)
	{
		CraftButton->SetIsEnabled(false);
	}
}

void ULSCraftingWidget::RefreshRecipeRows()
{
	ULSSaveSubsystem* SaveSubsystem = GetSaveSubsystem();
	for (ULSCraftingRowWidget* RecipeRow : ActiveRows)
	{
		const FRecipeEntry* Entry = RecipeRow ? FindRecipe(RecipeRow->GetRecipeRowName()) : nullptr;
		if (!RecipeRow || !Entry)
		{
			continue;
		}
		const LSInventorySlotUtils::FLSItemTradeInfo Info =
			LSInventorySlotUtils::ResolveItemTradeInfo(Entry->Recipe.ResultItemRowName);
		const int32 OwnedAmount = SaveSubsystem
			? SaveSubsystem->GetCraftingOwnedItemCount(Entry->Recipe.ResultItemRowName)
			: 0;
		RecipeRow->SetRecipe(Entry->RowName, Entry->Recipe.ResultItemRowName, Info.Name, OwnedAmount);
	}
}

void ULSCraftingWidget::RefreshDetail()
{
	const FRecipeEntry* SelectedRecipe = FindRecipe(SelectedRecipeRowName);
	ULSSaveSubsystem* SaveSubsystem = GetSaveSubsystem();
	if (!SelectedRecipe || !SaveSubsystem)
	{
		ClearSelection();
		return;
	}

	if (DetailPanel)
	{
		DetailPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	RefreshMaterialSlots(SelectedRecipe->Recipe, *SaveSubsystem);
	RefreshCraftingSummary(SelectedRecipe->Recipe, *SaveSubsystem);
}

void ULSCraftingWidget::RefreshMaterialSlots(
	const FLSCraftingRecipeRow& Recipe,
	ULSSaveSubsystem& SaveSubsystem)
{
	ActiveMaterialSlots.Reset();
	if (!MaterialList || !MaterialSlotClass)
	{
		return;
	}
	MaterialList->ClearChildren();

	for (const FLSCraftingMaterialCost& Material : Recipe.Materials)
	{
		ULSCraftingMaterialSlotWidget* MaterialSlot =
			CreateWidget<ULSCraftingMaterialSlotWidget>(this, MaterialSlotClass);
		if (!MaterialSlot)
		{
			UE_LOG(LogLS, Warning, TEXT("[Crafting] Failed to create material slot on %s."), *GetNameSafe(this));
			continue;
		}
		MaterialSlot->SetMaterial(
			Material.ItemRowName,
			SaveSubsystem.GetCraftingOwnedItemCount(Material.ItemRowName),
			Material.RequiredAmount);
		MaterialList->AddChildToWrapBox(MaterialSlot);
		ActiveMaterialSlots.Add(MaterialSlot);
	}
}

void ULSCraftingWidget::RefreshCraftingSummary(
	const FLSCraftingRecipeRow& Recipe,
	ULSSaveSubsystem& SaveSubsystem) const
{
	if (GoldCostText)
	{
		GoldCostText->SetText(FText::AsNumber(Recipe.GoldCost));
	}
	const int32 CraftableCount = SaveSubsystem.GetCraftableCount(Recipe);
	if (CraftableCountText)
	{
		CraftableCountText->SetText(FText::Format(
			LOCTEXT("CraftableCountFormat", "{0}개 제작 가능"),
			FText::AsNumber(CraftableCount)));
	}
	if (CraftButton)
	{
		CraftButton->SetIsEnabled(CraftableCount > 0);
	}
}

const ULSCraftingWidget::FRecipeEntry* ULSCraftingWidget::FindRecipe(const FName RecipeRowName) const
{
	return Recipes.FindByPredicate([RecipeRowName](const FRecipeEntry& Entry)
	{
		return Entry.RowName == RecipeRowName;
	});
}

ULSSaveSubsystem* ULSCraftingWidget::GetSaveSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
}

#undef LOCTEXT_NAMESPACE
