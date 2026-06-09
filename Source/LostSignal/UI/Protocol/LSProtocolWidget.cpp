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
	SynergyStageLevels.Reset();
}

void ULSProtocolWidget::SetProtocolStageLevels(const TArray<int32>& InSynergyStageLevels)
{
	SynergyStageLevels.Reset();
	for (const int32 StageLevel : InSynergyStageLevels)
	{
		if (StageLevel > 0)
		{
			SynergyStageLevels.AddUnique(StageLevel);
		}
	}

	SynergyStageLevels.Sort();
	SynergyStageCount = SynergyStageLevels.Num();
}

void ULSProtocolWidget::SetProtocolType(const ELSProtocolType InProtocolType)
{
	ProtocolType = InProtocolType;
	RefreshProtocolTooltip();
}

FString ULSProtocolWidget::BuildSynergyMarkup(int32 ActiveStage) const
{
	const int32 Count = FMath::Max(0, SynergyStageCount);
	const int32 Active = FMath::Clamp(ActiveStage, 0, SynergyStageLevels.IsEmpty() ? Count : MAX_int32);

	FString Markup;

	TArray<int32> DisplayLevels;
	if (SynergyStageLevels.IsEmpty())
	{
		for (int32 i = 1; i <= Count; ++i)
		{
			DisplayLevels.Add(i);
		}
	}
	else
	{
		DisplayLevels = SynergyStageLevels;
	}

	TArray<int32> ActiveLevels;
	TArray<int32> InactiveLevels;
	for (const int32 DisplayLevel : DisplayLevels)
	{
		if (DisplayLevel <= Active)
		{
			ActiveLevels.Add(DisplayLevel);
		}
		else
		{
			InactiveLevels.Add(DisplayLevel);
		}
	}

	if (!ActiveLevels.IsEmpty())
	{
		Markup += TEXT("<Bold>");
		for (int32 Index = 0; Index < ActiveLevels.Num(); ++Index)
		{
			if (Index > 0)
			{
				Markup += TEXT("/");
			}
			Markup += FString::FromInt(ActiveLevels[Index]);
		}
		Markup += TEXT("</>");
	}

	if (!InactiveLevels.IsEmpty())
	{
		Markup += TEXT("<Light>");
		for (int32 Index = 0; Index < InactiveLevels.Num(); ++Index)
		{
			if (!ActiveLevels.IsEmpty() || Index > 0)
			{
				Markup += TEXT("/");
			}
			Markup += FString::FromInt(InactiveLevels[Index]);
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
