#include "UI/Lobby/Store/LSVendingSlotWidget.h"

#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "LostSignal.h"
#include "UI/Inventory/LSItemSlotWidget.h"

void ULSVendingSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	const TCHAR* WidgetNames[] = { TEXT("SlotBorder"), TEXT("ItemSlot"), TEXT("PriceBox"), TEXT("PriceText") };
	const UWidget* Widgets[] = { SlotBorder, ItemSlot, PriceBox, PriceText };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Widgets); ++Index)
	{
		if (!Widgets[Index])
		{
			UE_LOG(LogLS, Warning, TEXT("[Store] %s is not bound on %s."), WidgetNames[Index], *GetNameSafe(this));
		}
	}

	if (SlotBorder)
	{
		NormalBorderColor = SlotBorder->GetBrushColor();
	}
}

FReply ULSVendingSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		OnClicked.Broadcast(this);
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void ULSVendingSlotWidget::SetStockItem(const FName InItemRowName, const int32 InPrice)
{
	ItemRowName = InItemRowName;
	Price = InPrice;
	Amount = 1;
	bStockSlot = true;
	SlotIndex = INDEX_NONE;

	if (ItemSlot)
	{
		ItemSlot->SetDisplayOnlySlotContext();
		ItemSlot->SetItem(InItemRowName, 1, TArray<FLSChipResolvedStat>());
	}
	if (PriceBox)
	{
		PriceBox->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (PriceText)
	{
		PriceText->SetText(FText::AsNumber(InPrice));
	}
}

void ULSVendingSlotWidget::SetOwnedItem(const FName InItemRowName, const int32 InAmount, const ELSInventorySlotArea InArea, const int32 InSlotIndex)
{
	ItemRowName = InItemRowName;
	Price = 0;
	Amount = InAmount;
	bStockSlot = false;
	Area = InArea;
	SlotIndex = InSlotIndex;

	if (ItemSlot)
	{
		ItemSlot->SetDisplayOnlySlotContext();
		ItemSlot->SetItem(InItemRowName, InAmount, TArray<FLSChipResolvedStat>());
	}
	if (PriceBox)
	{
		PriceBox->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ULSVendingSlotWidget::SetSelected(const bool bInSelected)
{
	if (SlotBorder)
	{
		SlotBorder->SetBrushColor(bInSelected ? SelectedBorderColor : NormalBorderColor);
	}
}
