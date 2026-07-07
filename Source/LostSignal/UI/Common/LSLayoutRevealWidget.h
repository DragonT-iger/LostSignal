#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LSLayoutRevealWidget.generated.h"

// 열리는 첫 프레임의 레이아웃 튐(깜빡임)을 숨기는 공용 베이스.
//
// 원인: 자동 줄바꿈 텍스트·WrapBox 등은 첫 프레임에 실제 할당 폭을 모른 채 DesiredSize를 계산하고
// 다음 프레임에 재계산한다. Collapsed 상태에서는 레이아웃 계산이 아예 돌지 않아, 다시 보이는
// 첫 프레임에도 같은 문제가 생긴다. 그래서 위젯이 "보이게 되는" 첫 1~2프레임 동안 RenderOpacity를
// 0으로 숨겼다가 레이아웃이 안정된 뒤 공개한다.
//
// 적용 지점: 처음 뷰포트에 붙을 때(NativeConstruct)와, 숨김(Collapsed/Hidden) → 표시 전환(SetVisibility).
// Visibility 대신 RenderOpacity를 쓰는 이유: 숨긴 프레임에도 레이아웃 계산·포커스는 그대로 돌아야 한다.
// 주의: 이 베이스를 쓰는 위젯의 WBP에서 루트 RenderOpacity를 애니메이션으로 건드리면 공개 타이밍과 충돌한다.
UCLASS(Abstract)
class LOSTSIGNAL_API ULSLayoutRevealWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void SetVisibility(ESlateVisibility InVisibility) override;

private:
	// 숨김 → 표시 전환 시 호출. 투명하게 만들고 공개 카운트를 시작한다.
	void BeginLayoutReveal();

	// 공개까지 남은 프레임 수. 0이면 공개 완료 상태.
	int32 RevealFramesRemaining = 0;
};
