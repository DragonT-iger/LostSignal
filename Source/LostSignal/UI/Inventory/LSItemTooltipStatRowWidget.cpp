#include "UI/Inventory/LSItemTooltipStatRowWidget.h"

#include "Components/TextBlock.h"
#include "LostSignal.h"

void ULSItemTooltipStatRowWidget::SetStat(const FText& StatName, const FText& StatValue)
{
	if (!StatNameText)
	{
		UE_LOG(LogLS, Warning, TEXT("StatNameText is not bound on %s."), *GetNameSafe(this));
		return;
	}

	if (!StatValueText)
	{
		UE_LOG(LogLS, Warning, TEXT("StatValueText is not bound on %s."), *GetNameSafe(this));
		return;
	}

	StatNameText->SetText(StatName);
	StatValueText->SetText(StatValue);
}
