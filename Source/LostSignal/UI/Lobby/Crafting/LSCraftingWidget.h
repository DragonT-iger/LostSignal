#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/LSCraftingRecipeRow.h"
#include "LSCraftingWidget.generated.h"

class UButton;
class UScrollBox;
class UTextBlock;
class UWrapBox;
class ULSSaveSubsystem;
class ULSCraftingMaterialSlotWidget;
class ULSCraftingRowWidget;
class ULSCraftingTabWidget;

UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSCraftingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 제작 화면을 열 때마다 레시피와 저장 데이터를 다시 읽는다.
	void OpenCrafting();

protected:
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Crafting")
	TObjectPtr<ULSCraftingTabWidget> AllTab;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Crafting")
	TObjectPtr<ULSCraftingTabWidget> WeaponTab;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Crafting")
	TObjectPtr<ULSCraftingTabWidget> ArmorTab;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Crafting")
	TObjectPtr<ULSCraftingTabWidget> ChipTab;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Crafting")
	TObjectPtr<ULSCraftingTabWidget> ConsumableTab;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Crafting")
	TObjectPtr<UScrollBox> RecipeList;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Crafting")
	TObjectPtr<UWidget> DetailPanel;

	// 레시피의 재료 수만큼 MaterialSlotClass를 동적으로 채우는 컨테이너.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Crafting")
	TObjectPtr<UWrapBox> MaterialList;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Crafting")
	TObjectPtr<UTextBlock> GoldCostText;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Crafting")
	TObjectPtr<UTextBlock> CraftableCountText;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Crafting")
	TObjectPtr<UButton> CraftButton;

	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Crafting")
	TSubclassOf<ULSCraftingRowWidget> CraftingRowClass;

	UPROPERTY(EditDefaultsOnly, Category="LS/UI|Crafting")
	TSubclassOf<ULSCraftingMaterialSlotWidget> MaterialSlotClass;

private:
	struct FRecipeEntry
	{
		FName RowName;
		FLSCraftingRecipeRow Recipe;
	};

	UFUNCTION()
	void HandleTabClicked(ULSCraftingTabWidget* ClickedTab);

	UFUNCTION()
	void HandleRecipeClicked(ULSCraftingRowWidget* ClickedRow);

	UFUNCTION()
	void HandleCraftClicked();

	void LoadRecipes();
	void SetCategory(ELSCraftingCategory NewCategory);
	void RebuildRecipeList();
	void ClearRecipeRows();
	ULSCraftingRowWidget* CreateRecipeRow(const FRecipeEntry& Entry, ULSSaveSubsystem* SaveSubsystem);
	void SelectRecipe(FName RecipeRowName);
	void ClearSelection();
	void RefreshRecipeRows();
	void RefreshDetail();
	void RefreshMaterialSlots(const FLSCraftingRecipeRow& Recipe, ULSSaveSubsystem& SaveSubsystem);
	void RefreshCraftingSummary(const FLSCraftingRecipeRow& Recipe, ULSSaveSubsystem& SaveSubsystem) const;
	const FRecipeEntry* FindRecipe(FName RecipeRowName) const;
	ULSSaveSubsystem* GetSaveSubsystem() const;

	TArray<FRecipeEntry> Recipes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULSCraftingRowWidget>> ActiveRows;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULSCraftingMaterialSlotWidget>> ActiveMaterialSlots;

	FName SelectedRecipeRowName;
	ELSCraftingCategory CurrentCategory = ELSCraftingCategory::All;
};
