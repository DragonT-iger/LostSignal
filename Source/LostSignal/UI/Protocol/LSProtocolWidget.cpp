#include "UI/Protocol/LSProtocolWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/RichTextBlock.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "LostSignal.h"
#include "UI/Protocol/LSProtocolTooltipWidget.h"

void ULSProtocolWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RefreshProtocolTooltip();
}

void ULSProtocolWidget::SetProtocol(int32 Level, int32 SynergyStage)
{
	SetProtocolLevels(Level, Level, SynergyStage);
}

void ULSProtocolWidget::SetProtocolLevels(const int32 CurrentLevel, const int32 PreviousLevel, const int32 SynergyStage)
{
	if (LevelText)
	{
		LevelText->SetText(FText::AsNumber(CurrentLevel));
	}
	if (SynergyStageText)
	{
		SynergyStageText->SetText(FText::FromString(BuildSynergyMarkup(SynergyStage)));
	}

	CurrentProtocolLevel = CurrentLevel;
	PreviousProtocolLevel = PreviousLevel;
	RefreshProtocolTooltip();
}

void ULSProtocolWidget::SetProtocolStageCount(const int32 InSynergyStageCount)
{
	SynergyStageCount = FMath::Max(0, InSynergyStageCount);
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

ULSProtocolTooltipWidget* ULSProtocolWidget::CreateProtocolTooltipWidget()
{
	if (!ProtocolTooltipWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("ProtocolTooltipWidgetClass is not set on %s."), *GetNameSafe(this));
		return nullptr;
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
		return nullptr;
	}

	ProtocolTooltipWidget->SetProtocolTooltipLevels(ProtocolType, TooltipIconTexture, CurrentProtocolLevel, PreviousProtocolLevel);
	return ProtocolTooltipWidget;
}

void ULSProtocolWidget::RefreshProtocolTooltip()
{
	ULSProtocolTooltipWidget* ProtocolTooltipWidget = CreateProtocolTooltipWidget();
	if (!ProtocolTooltipWidget)
	{
		SetToolTip(nullptr);
		return;
	}

	SetToolTip(ProtocolTooltipWidget);

	UWidget* RootWidget = WidgetTree ? WidgetTree->RootWidget : nullptr;
	if (!RootWidget)
	{
		UE_LOG(LogLS, Warning, TEXT("RootWidget is not set on %s."), *GetNameSafe(this));
		return;
	}

	if (RootWidget->GetVisibility() == ESlateVisibility::HitTestInvisible ||
		RootWidget->GetVisibility() == ESlateVisibility::SelfHitTestInvisible)
	{
		RootWidget->SetVisibility(ESlateVisibility::Visible);
	}

	if (ULSProtocolTooltipWidget* RootTooltipWidget = CreateProtocolTooltipWidget())
	{
		RootWidget->SetToolTip(RootTooltipWidget);
	}
}
