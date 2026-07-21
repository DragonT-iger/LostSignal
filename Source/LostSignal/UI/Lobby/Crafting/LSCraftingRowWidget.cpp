#include "UI/Lobby/Crafting/LSCraftingRowWidget.h"

#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"
#include "LostSignal.h"
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
	}
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
	const int32 OwnedAmount)
{
	RecipeRowName = InRecipeRowName;
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
}

void ULSCraftingRowWidget::SetSelected(const bool bSelected) const
{
	if (SelectionBorder)
	{
		SelectionBorder->SetBrushColor(bSelected ? SelectedBorderColor : NormalBorderColor);
	}
}
