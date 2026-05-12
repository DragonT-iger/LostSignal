#include "UI/Inventory/LSItemTooltipExtraInfoRowWidget.h"

#include "Components/TextBlock.h"
#include "LostSignal.h"

void ULSItemTooltipExtraInfoRowWidget::SetExtraInfo(const FText& ExtraInfoName, const FText& ExtraInfoValue)
{
	if (!ExtraInfoNameText)
	{
		UE_LOG(LogLS, Warning, TEXT("ExtraInfoNameText is not bound on %s."), *GetNameSafe(this));
		return;
	}

	if (!ExtraInfoValueText)
	{
		UE_LOG(LogLS, Warning, TEXT("ExtraInfoValueText is not bound on %s."), *GetNameSafe(this));
		return;
	}

	ExtraInfoNameText->SetText(ExtraInfoName);
	ExtraInfoValueText->SetText(ExtraInfoValue);
}
