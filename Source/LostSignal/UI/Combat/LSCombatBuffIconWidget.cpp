#include "UI/Combat/LSCombatBuffIconWidget.h"

#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "LostSignal.h"

void ULSCombatBuffIconWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!IconImage || !StackText || !DurationBar)
	{
		UE_LOG(LogLS, Warning, TEXT("%s is missing required combat buff icon binding. Icon=%s Stack=%s Bar=%s"),
			*GetNameSafe(this),
			*GetNameSafe(IconImage),
			*GetNameSafe(StackText),
			*GetNameSafe(DurationBar));
	}
}

void ULSCombatBuffIconWidget::SetBuffDisplay(const FLSCombatBuffDisplayData& InDisplayData)
{
	if (IconImage)
	{
		if (InDisplayData.IconTexture)
		{
			IconImage->SetBrushFromTexture(InDisplayData.IconTexture);
			IconImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			IconImage->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (StackText)
	{
		StackText->SetText(InDisplayData.StackCount > 1 ? FText::AsNumber(InDisplayData.StackCount) : FText::GetEmpty());
	}

	if (DurationBar)
	{
		const float Percent = InDisplayData.TotalDuration > 0.0f
			? FMath::Clamp(InDisplayData.RemainingTime / InDisplayData.TotalDuration, 0.0f, 1.0f)
			: 0.0f;
		DurationBar->SetPercent(Percent);
	}
}
