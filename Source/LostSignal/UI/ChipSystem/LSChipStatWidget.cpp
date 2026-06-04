#include "UI/ChipSystem/LSChipStatWidget.h"

#include "Components/Border.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "LostSignal.h"

void ULSChipStatWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bHasAnimatedState || !bIsAnimating)
	{
		return;
	}

	AnimationElapsed += InDeltaTime;
	const float Alpha = GetAnimationAlpha();

	AnimatedStatValue = FMath::Lerp(StartStatValue, static_cast<float>(TargetStatValue), Alpha);
	AnimatedSignalLoss = FMath::Lerp(StartSignalLoss, static_cast<float>(TargetSignalLoss), Alpha);
	AnimatedStatFill = FMath::Lerp(StartStatFill, TargetStatFill, Alpha);
	AnimatedSignalFill = FMath::Lerp(StartSignalFill, TargetSignalFill, Alpha);
	AnimatedEmptyFill = FMath::Lerp(StartEmptyFill, TargetEmptyFill, Alpha);

	if (Alpha >= 1.f)
	{
		SetAnimatedValuesToTarget();
		bIsAnimating = false;
	}

	ApplyAnimatedValues();
}

void ULSChipStatWidget::SetStat(const FText& StatName, const int32 StatValue, const int32 SignalLoss)
{
	const bool bWasInitialized = bHasAnimatedState;
	const int32 PreviousTargetStatValue = TargetStatValue;
	const int32 PreviousTargetSignalLoss = TargetSignalLoss;
	TargetStatValue = FMath::Max(0, StatValue);
	TargetSignalLoss = FMath::Max(0, SignalLoss);

	const int32 Max = FMath::Max(0, GaugeMax);
	TargetStatFill = FMath::Clamp(static_cast<float>(TargetStatValue), 0.f, static_cast<float>(Max));
	TargetSignalFill = FMath::Clamp(static_cast<float>(TargetSignalLoss), 0.f, static_cast<float>(Max) - TargetStatFill);
	TargetEmptyFill = FMath::Max(0.f, static_cast<float>(Max) - TargetStatFill - TargetSignalFill);

	if (StatNameText)
	{
		StatNameText->SetText(StatName);
	}

	if (!bHasAnimatedState)
	{
		bHasAnimatedState = true;
		SetAnimatedValuesToTarget();
	}
	else if (bWasInitialized)
	{
		const bool bTargetChanged = PreviousTargetStatValue != TargetStatValue || PreviousTargetSignalLoss != TargetSignalLoss;
		if (bTargetChanged)
		{
			CacheAnimationStartValues();
			AnimationElapsed = 0.f;
			bIsAnimating = true;
		}
	}

	ApplyAnimatedValues();
}

void ULSChipStatWidget::ApplyAnimatedValues()
{
	SetDisplayedTexts(
		FMath::Max(0, FMath::RoundToInt(AnimatedStatValue)),
		FMath::Max(0, FMath::RoundToInt(AnimatedSignalLoss)));

	SetGaugeFillWeight(GaugeStat, AnimatedStatFill);
	SetGaugeFillWeight(GaugeSignal, AnimatedSignalFill);
	SetGaugeFillWeight(GaugeEmpty, AnimatedEmptyFill);
}

void ULSChipStatWidget::SetDisplayedTexts(const int32 StatValue, const int32 SignalLoss) const
{
	if (StatValueText)
	{
		StatValueText->SetText(FText::Format(NSLOCTEXT("LSChipStat", "StatValueFmt", "+{0}"), StatValue));
	}
	if (SignalLossText)
	{
		SignalLossText->SetText(FText::Format(NSLOCTEXT("LSChipStat", "SignalLossFmt", "+{0}"), SignalLoss));
	}
}

void ULSChipStatWidget::SetAnimatedValuesToTarget()
{
	const int32 Max = FMath::Max(0, GaugeMax);
	AnimatedStatValue = static_cast<float>(TargetStatValue);
	AnimatedSignalLoss = static_cast<float>(TargetSignalLoss);
	TargetStatFill = FMath::Clamp(static_cast<float>(TargetStatValue), 0.f, static_cast<float>(Max));
	TargetSignalFill = FMath::Clamp(static_cast<float>(TargetSignalLoss), 0.f, static_cast<float>(Max) - TargetStatFill);
	TargetEmptyFill = FMath::Max(0.f, static_cast<float>(Max) - TargetStatFill - TargetSignalFill);
	AnimatedStatFill = TargetStatFill;
	AnimatedSignalFill = TargetSignalFill;
	AnimatedEmptyFill = TargetEmptyFill;
	CacheAnimationStartValues();
	AnimationElapsed = FMath::Max(0.f, StatAnimationDuration);
	bIsAnimating = false;
}

void ULSChipStatWidget::CacheAnimationStartValues()
{
	StartStatValue = AnimatedStatValue;
	StartSignalLoss = AnimatedSignalLoss;
	StartStatFill = AnimatedStatFill;
	StartSignalFill = AnimatedSignalFill;
	StartEmptyFill = AnimatedEmptyFill;
}

float ULSChipStatWidget::GetAnimationAlpha() const
{
	const float Duration = FMath::Max(KINDA_SMALL_NUMBER, StatAnimationDuration);
	const float LinearAlpha = FMath::Clamp(AnimationElapsed / Duration, 0.f, 1.f);
	return FMath::InterpEaseInOut(0.f, 1.f, LinearAlpha, 2.f);
}

void ULSChipStatWidget::SetGaugeFillWeight(UBorder* Segment, const float Weight)
{
	if (!Segment)
	{
		return;
	}

	UHorizontalBoxSlot* BoxSlot = Cast<UHorizontalBoxSlot>(Segment->Slot);
	if (!BoxSlot)
	{
		UE_LOG(LogLS, Warning, TEXT("[ChipStat] %s must be a child of HorizontalBox to apply gauge fill weight."),
			*GetNameSafe(Segment));
		return;
	}

	FSlateChildSize NewSize(ESlateSizeRule::Fill);
	NewSize.Value = FMath::Max(0.f, Weight);
	BoxSlot->SetSize(NewSize);
}
