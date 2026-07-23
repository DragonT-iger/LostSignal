#include "UI/Combat/LSCombatBuffIconWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "LostSignal.h"
#include "Materials/MaterialInstanceDynamic.h"

void ULSCombatBuffIconWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!IconImage || !StackText || !DurationMaskImage)
	{
		UE_LOG(LogLS, Warning, TEXT("%s is missing required combat buff icon binding. Icon=%s Stack=%s DurationMaskImage=%s"),
			*GetNameSafe(this),
			*GetNameSafe(IconImage),
			*GetNameSafe(StackText),
			*GetNameSafe(DurationMaskImage));
		return;
	}

	DurationMaskMaterial = DurationMaskImage->GetDynamicMaterial();
	if (!DurationMaskMaterial)
	{
		UE_LOG(LogLS, Warning, TEXT("%s cannot create buff duration mask material. Check DurationMaskImage brush material."), *GetNameSafe(this));
	}
}

void ULSCombatBuffIconWidget::SetBuffDisplay(const FLSCombatBuffDisplayData& InDisplayData)
{
	if (IconImage)
	{
		if (InDisplayData.IconTexture)
		{
			IconImage->SetBrushFromTexture(InDisplayData.IconTexture);
		}

		IconImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	if (StackText)
	{
		StackText->SetText(InDisplayData.StackCount > 1 ? FText::AsNumber(InDisplayData.StackCount) : FText::GetEmpty());
	}

	if (DurationMaskMaterial)
	{
		const float RemainingRatio = InDisplayData.TotalDuration > 0.0f
			? FMath::Clamp(InDisplayData.RemainingTime / InDisplayData.TotalDuration, 0.0f, 1.0f)
			: 0.0f;
		const float FillProgress = bBuffFillByElapsed ? (1.0f - RemainingRatio) : RemainingRatio;
		DurationMaskMaterial->SetScalarParameterValue(BuffProgressParameterName, FillProgress);
	}
}
