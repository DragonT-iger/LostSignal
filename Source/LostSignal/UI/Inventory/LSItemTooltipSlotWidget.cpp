#include "UI/Inventory/LSItemTooltipSlotWidget.h"

#include "LostSignal.h"
#include "UI/Inventory/LSItemTooltipWidget.h"

void ULSItemTooltipSlotWidget::SetTooltipItem(const FName ItemRowName, const int32 Amount)
{
	CurrentTooltipItemRowName = ItemRowName;
	CurrentTooltipAmount = Amount;
	bHasTooltipItem = !ItemRowName.IsNone();
}

void ULSItemTooltipSlotWidget::ClearTooltipItem()
{
	CurrentTooltipItemRowName = NAME_None;
	CurrentTooltipAmount = 0;
	bHasTooltipItem = false;
	HideItemTooltip();
}

void ULSItemTooltipSlotWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	ShowItemTooltip();
}

void ULSItemTooltipSlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	HideItemTooltip();
	Super::NativeOnMouseLeave(InMouseEvent);
}

void ULSItemTooltipSlotWidget::ShowItemTooltip()
{
	if (!HasTooltipItem())
	{
		return;
	}

	if (!ItemTooltipWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("ItemTooltipWidgetClass is not set on %s."), *GetNameSafe(this));
		return;
	}

	if (!ItemTooltipWidget)
	{
		if (APlayerController* OwningPlayer = GetOwningPlayer())
		{
			ItemTooltipWidget = CreateWidget<ULSItemTooltipWidget>(OwningPlayer, ItemTooltipWidgetClass);
		}
		else if (UWorld* World = GetWorld())
		{
			ItemTooltipWidget = CreateWidget<ULSItemTooltipWidget>(World, ItemTooltipWidgetClass);
		}
	}

	if (!ItemTooltipWidget)
	{
		UE_LOG(LogLS, Warning, TEXT("Failed to create item tooltip widget on %s."), *GetNameSafe(this));
		return;
	}

	ItemTooltipWidget->SetItem(CurrentTooltipItemRowName, CurrentTooltipAmount);
	SetToolTip(ItemTooltipWidget);
}

void ULSItemTooltipSlotWidget::HideItemTooltip()
{
	SetToolTip(nullptr);
}
