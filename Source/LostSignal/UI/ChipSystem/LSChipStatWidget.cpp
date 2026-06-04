#include "UI/ChipSystem/LSChipStatWidget.h"

#include "Components/Border.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "LostSignal.h"

void ULSChipStatWidget::SetStat(const FText& StatName, int32 StatValue, int32 SignalLoss)
{
	UE_LOG(LogLS, Warning,
	       TEXT("[ChipStat] SetStat '%s' (Stat=%d, Signal=%d) | NameText=%d ValueText=%d SignalText=%d | GaugeStat=%d GaugeSignal=%d GaugeEmpty=%d"),
	       *StatName.ToString(), StatValue, SignalLoss,
	       StatNameText != nullptr, StatValueText != nullptr, SignalLossText != nullptr,
	       GaugeStat != nullptr, GaugeSignal != nullptr, GaugeEmpty != nullptr);

	const int32 Max = FMath::Max(0, GaugeMax);
	const int32 StatFill = FMath::Clamp(StatValue, 0, Max);
	const int32 SignalFill = FMath::Clamp(SignalLoss, 0, Max - StatFill);
	const int32 EmptyFill = FMath::Max(0, Max - StatFill - SignalFill);

	// ---- 텍스트 ----
	if (StatNameText)
	{
		StatNameText->SetText(StatName);
	}
	if (StatValueText)
	{
		StatValueText->SetText(
			FText::Format(NSLOCTEXT("LSChipStat", "StatValueFmt", "+{0}"), FMath::Max(0, StatValue)));
	}
	if (SignalLossText)
	{
		SignalLossText->SetText(
			FText::Format(NSLOCTEXT("LSChipStat", "SignalLossFmt", "+{0}"), FMath::Max(0, SignalLoss)));
	}

	// ---- 게이지 바: [파랑=StatValue][분홍=SignalLoss][빈칸=나머지] ----
	SetGaugeFillWeight(GaugeStat, static_cast<float>(StatFill));
	SetGaugeFillWeight(GaugeSignal, static_cast<float>(SignalFill));
	SetGaugeFillWeight(GaugeEmpty, static_cast<float>(EmptyFill));
}

void ULSChipStatWidget::SetGaugeFillWeight(UBorder* Segment, float Weight)
{
	if (!Segment)
	{
		return;
	}

	UHorizontalBoxSlot* BoxSlot = Cast<UHorizontalBoxSlot>(Segment->Slot);
	if (!BoxSlot)
	{
		UE_LOG(LogLS, Warning, TEXT("[ChipStat] %s 는 HorizontalBox 의 자식이어야 게이지 비중을 적용할 수 있습니다."),
		       *GetNameSafe(Segment));
		return;
	}

	FSlateChildSize NewSize(ESlateSizeRule::Fill);
	NewSize.Value = FMath::Max(0.f, Weight);
	BoxSlot->SetSize(NewSize);
}
