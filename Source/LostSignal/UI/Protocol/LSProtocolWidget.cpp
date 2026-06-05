#include "UI/Protocol/LSProtocolWidget.h"

#include "Components/RichTextBlock.h"
#include "Components/TextBlock.h"
#include "LostSignal.h"
#include "UI/Protocol/LSProtocolTooltipWidget.h"

void ULSProtocolWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RefreshProtocolTooltip();
}

void ULSProtocolWidget::SetProtocol(int32 Level, int32 SynergyStage)
{
	if (LevelText)
	{
		LevelText->SetText(FText::AsNumber(Level));
	}
	if (SynergyStageText)
	{
		SynergyStageText->SetText(FText::FromString(BuildSynergyMarkup(SynergyStage)));
	}
}

void ULSProtocolWidget::SetProtocolType(const ELSProtocolType InProtocolType)
{
	ProtocolType = InProtocolType;
	RefreshProtocolTooltip();
}

FString ULSProtocolWidget::BuildSynergyMarkup(int32 ActiveStage) const
{
	const int32 Count = FMath::Max(0, SynergyStageCount);
	const int32 Active = FMath::Clamp(ActiveStage, 0, Count);

	FString Markup;

	// 활성 단계: <Bold>1/2/.../Active</>
	if (Active >= 1)
	{
		Markup += TEXT("<Bold>");
		for (int32 i = 1; i <= Active; ++i)
		{
			if (i > 1)
			{
				Markup += TEXT("/");
			}
			Markup += FString::FromInt(i);
		}
		Markup += TEXT("</>");
	}

	// 비활성 단계: <Light>/Active+1/.../Count</> (Bold 경계 슬래시는 Light 쪽에 둔다)
	if (Active < Count)
	{
		Markup += TEXT("<Light>");
		for (int32 i = Active + 1; i <= Count; ++i)
		{
			if (i > 1)
			{
				Markup += TEXT("/");
			}
			Markup += FString::FromInt(i);
		}
		Markup += TEXT("</>");
	}

	return Markup;
}

void ULSProtocolWidget::RefreshProtocolTooltip()
{
	if (!ProtocolTooltipWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("ProtocolTooltipWidgetClass is not set on %s."), *GetNameSafe(this));
		SetToolTip(nullptr);
		return;
	}

	ULSProtocolTooltipWidget* ProtocolTooltipWidget = nullptr;
	if (APlayerController* OwningPlayer = GetOwningPlayer())
	{
		ProtocolTooltipWidget = CreateWidget<ULSProtocolTooltipWidget>(OwningPlayer, ProtocolTooltipWidgetClass);
	}
	else if (UWorld* World = GetWorld())
	{
		ProtocolTooltipWidget = CreateWidget<ULSProtocolTooltipWidget>(World, ProtocolTooltipWidgetClass);
	}

	if (!ProtocolTooltipWidget)
	{
		UE_LOG(LogLS, Warning, TEXT("Failed to create protocol tooltip widget on %s."), *GetNameSafe(this));
		SetToolTip(nullptr);
		return;
	}

	ProtocolTooltipWidget->SetProtocolTooltip(ProtocolType, TooltipIconTexture);
	SetToolTip(ProtocolTooltipWidget);
}
