#include "UI/Inventory/LSItemTooltipSlotWidget.h"

#include "LostSignal.h"
#include "UI/Inventory/LSItemTooltipWidget.h"

void ULSItemTooltipSlotWidget::SetTooltipItem(const FName ItemRowName, const int32 Amount, const int32 StatSeed)
{
	CurrentTooltipItemRowName = ItemRowName;
	CurrentTooltipAmount = Amount;
	CurrentTooltipStatSeed = StatSeed;
	bHasTooltipItem = !ItemRowName.IsNone();
	RefreshItemTooltip();
}

void ULSItemTooltipSlotWidget::ClearTooltipItem()
{
	CurrentTooltipItemRowName = NAME_None;
	CurrentTooltipAmount = 0;
	CurrentTooltipStatSeed = 0;
	bHasTooltipItem = false;
	SetToolTip(nullptr);
}

void ULSItemTooltipSlotWidget::RefreshItemTooltip()
{
	if (!HasTooltipItem())
	{
		SetToolTip(nullptr);
		return;
	}

	if (!ItemTooltipWidget)
	{
		if (!ItemTooltipWidgetClass)
		{
			UE_LOG(LogLS, Warning, TEXT("ItemTooltipWidgetClass is not set on %s."), *GetNameSafe(this));
			return;
		}

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

	ItemTooltipWidget->SetItem(CurrentTooltipItemRowName, CurrentTooltipAmount, CurrentTooltipStatSeed);
	SetToolTip(ItemTooltipWidget);
}
