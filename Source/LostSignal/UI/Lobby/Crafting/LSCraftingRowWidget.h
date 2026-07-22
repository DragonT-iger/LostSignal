#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LSCraftingRowWidget.generated.h"

class UBorder;
class UTextBlock;
class ULSCraftingRowWidget;
class ULSItemSlotWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLSCraftingRowClicked, ULSCraftingRowWidget*, ClickedRow);

UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSCraftingRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual int32 NativePaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UPROPERTY(BlueprintAssignable, Category="LS/UI|Crafting")
	FLSCraftingRowClicked OnClicked;

	void SetRecipe(
		FName InRecipeRowName,
		FName ResultItemRowName,
		const FText& ItemName,
		int32 OwnedAmount,
		bool bInCraftable);
	void SetSelected(bool bSelected);
	FName GetRecipeRowName() const { return RecipeRowName; }

protected:
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Crafting")
	TObjectPtr<UBorder> SelectionBorder;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Crafting")
	TObjectPtr<ULSItemSlotWidget> ItemSlot;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Crafting")
	TObjectPtr<UTextBlock> ItemNameText;

	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Crafting")
	TObjectPtr<UTextBlock> OwnedCountText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|Crafting")
	FLinearColor SelectedBorderColor = FLinearColor(1.0f, 0.42f, 0.08f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|Crafting", meta=(ClampMin="0.0"))
	float SelectedBorderWidth = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|Crafting", meta=(ClampMin="0.0"))
	float SelectedBorderRadius = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/UI|Crafting")
	FLinearColor CraftableBorderColor = FLinearColor(0.22f, 0.85f, 0.35f, 1.0f);

private:
	void RefreshBackgroundColor() const;

	FName RecipeRowName;
	FLinearColor NormalBorderColor = FLinearColor::White;
	bool bCraftable = false;
	bool bIsSelected = false;
};
