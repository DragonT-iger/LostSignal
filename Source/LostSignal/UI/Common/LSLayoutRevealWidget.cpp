#include "UI/Common/LSLayoutRevealWidget.h"

namespace
{
	bool LSLayoutRevealIsShown(const ESlateVisibility Visibility)
	{
		return Visibility != ESlateVisibility::Collapsed && Visibility != ESlateVisibility::Hidden;
	}
}

void ULSLayoutRevealWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 처음 뷰포트에 붙는 프레임도 레이아웃 미확정 상태라 같은 튐이 보인다.
	BeginLayoutReveal();
}

void ULSLayoutRevealWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (RevealFramesRemaining > 0 && --RevealFramesRemaining == 0)
	{
		SetRenderOpacity(1.f);
	}
}

void ULSLayoutRevealWidget::SetVisibility(const ESlateVisibility InVisibility)
{
	const bool bWasShown = LSLayoutRevealIsShown(GetVisibility());
	Super::SetVisibility(InVisibility);

	// 숨김 → 표시 전환에서만 시작한다. 표시 중 상태 변경(예: Visible ↔ SelfHitTestInvisible)은 해당 없음.
	if (!bWasShown && LSLayoutRevealIsShown(InVisibility))
	{
		BeginLayoutReveal();
	}
}

void ULSLayoutRevealWidget::BeginLayoutReveal()
{
	SetRenderOpacity(0.f);
	// 1프레임째는 잘못된 레이아웃으로 그려질 수 있어 2프레임을 숨긴다. (60fps 기준 약 33ms)
	RevealFramesRemaining = 2;
}
