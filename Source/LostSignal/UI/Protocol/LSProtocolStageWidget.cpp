#include "UI/Protocol/LSProtocolStageWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "LostSignal.h"

void ULSProtocolStageWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!StageImage)
	{
		UE_LOG(LogLS, Warning, TEXT("StageImage is not bound on %s."), *GetNameSafe(this));
	}
	if (!StageText)
	{
		UE_LOG(LogLS, Warning, TEXT("StageText is not bound on %s."), *GetNameSafe(this));
	}
}

void ULSProtocolStageWidget::SetProtocolStage(const int32 StageOrder, const bool bUnlocked)
{
	if (StageImage)
	{
		StageImage->SetColorAndOpacity(bUnlocked ? UnlockedBoxColor : LockedBoxColor);
	}

	if (StageText)
	{
		StageText->SetText(FText::AsNumber(StageOrder));
		StageText->SetColorAndOpacity(bUnlocked ? UnlockedTextColor : LockedTextColor);
	}
}
