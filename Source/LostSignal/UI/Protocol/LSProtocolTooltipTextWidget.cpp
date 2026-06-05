#include "UI/Protocol/LSProtocolTooltipTextWidget.h"

#include "Components/TextBlock.h"
#include "LostSignal.h"

void ULSProtocolTooltipTextWidget::SetProtocolTooltipText(const FText& InText)
{
	if (!Text)
	{
		UE_LOG(LogLS, Warning, TEXT("Text is not bound on %s."), *GetNameSafe(this));
		return;
	}

	Text->SetText(InText);
}
