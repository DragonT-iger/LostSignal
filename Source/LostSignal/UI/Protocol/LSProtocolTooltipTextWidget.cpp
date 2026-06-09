#include "UI/Protocol/LSProtocolTooltipTextWidget.h"

#include "Components/TextBlock.h"
#include "LostSignal.h"

void ULSProtocolTooltipTextWidget::SetProtocolTooltipText(const FText& InText)
{
	SetProtocolTooltipStateText(InText, true, false);
}

void ULSProtocolTooltipTextWidget::SetProtocolTooltipStateText(const FText& InText, const bool bUnlocked, const bool bProtected)
{
	if (!Text)
	{
		UE_LOG(LogLS, Warning, TEXT("Text is not bound on %s."), *GetNameSafe(this));
		return;
	}

	Text->SetText(InText);
	Text->SetColorAndOpacity(bProtected ? ProtectedColor : (bUnlocked ? UnlockedColor : LockedColor));
}
