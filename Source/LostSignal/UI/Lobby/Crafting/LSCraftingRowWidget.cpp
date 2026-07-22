#include "UI/Lobby/Crafting/LSCraftingRowWidget.h"

#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"
#include "LostSignal.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Rendering/DrawElements.h"
#include "UI/Inventory/LSItemSlotWidget.h"

void ULSCraftingRowWidget::NativeConstruct()
{
	Super::NativeConstruct();

	const TCHAR* WidgetNames[] = { TEXT("SelectionBorder"), TEXT("ItemSlot"), TEXT("ItemNameText"), TEXT("OwnedCountText") };
	const UWidget* Widgets[] = { SelectionBorder, ItemSlot, ItemNameText, OwnedCountText };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Widgets); ++Index)
	{
		if (!Widgets[Index])
		{
			UE_LOG(LogLS, Warning, TEXT("[Crafting] %s is not bound on %s."), WidgetNames[Index], *GetNameSafe(this));
		}
	}

	if (SelectionBorder)
	{
		NormalBorderColor = SelectionBorder->GetBrushColor();
		RefreshBackgroundColor();
	}
}

int32 ULSCraftingRowWidget::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	const int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	const bool bParentEnabled) const
{
	const int32 MaxLayer = Super::NativePaint(
		Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	if (!bIsSelected || SelectedBorderWidth <= 0.0f)
	{
		return MaxLayer;
	}

	const FSlateRoundedBoxBrush OutlineBrush(
		FLinearColor::Transparent,
		SelectedBorderRadius,
		FSlateColor(SelectedBorderColor),
		SelectedBorderWidth);
	const ESlateDrawEffect DrawEffect = bParentEnabled && GetIsEnabled()
		? ESlateDrawEffect::None
		: ESlateDrawEffect::DisabledEffect;
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		MaxLayer + 1,
		AllottedGeometry.ToPaintGeometry(),
		&OutlineBrush,
		DrawEffect,
		OutlineBrush.GetTint(InWidgetStyle));
	return MaxLayer + 1;
}

FReply ULSCraftingRowWidget::NativeOnMouseButtonDown(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && !RecipeRowName.IsNone())
	{
		OnClicked.Broadcast(this);
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void ULSCraftingRowWidget::SetRecipe(
	const FName InRecipeRowName,
	const FName ResultItemRowName,
	const FText& ItemName,
	const int32 OwnedAmount,
	const bool bInCraftable)
{
	RecipeRowName = InRecipeRowName;
	bCraftable = bInCraftable;
	if (ItemSlot)
	{
		ItemSlot->SetDisplayOnlySlotContext();
		ItemSlot->SetItem(ResultItemRowName, OwnedAmount, TArray<FLSChipResolvedStat>());
		ItemSlot->SetAmountTextVisible(false);
	}
	if (ItemNameText)
	{
		ItemNameText->SetText(ItemName);
	}
	if (OwnedCountText)
	{
		OwnedCountText->SetText(FText::AsNumber(OwnedAmount));
	}
	RefreshBackgroundColor();
}

void ULSCraftingRowWidget::SetSelected(const bool bSelected)
{
	if (bIsSelected == bSelected)
	{
		return;
	}
	bIsSelected = bSelected;
	Invalidate(EInvalidateWidgetReason::Paint);
}

void ULSCraftingRowWidget::RefreshBackgroundColor() const
{
	if (SelectionBorder)
	{
		SelectionBorder->SetBrushColor(bCraftable ? CraftableBorderColor : NormalBorderColor);
	}
}
