#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LSDistanceMarkerWidget.generated.h"

// 거리 기반 빌보드 마커의 C++ 부모. 아트는 이 클래스를 상속해 WBP_* 원 UI를 만든다.
UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSDistanceMarkerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 컴포넌트가 매 틱 거리 비율을 전달한다(0=근거리 완전 표시, 1=최대 거리). 아트가 링 연출에 쓰고 싶으면 사용(선택).
	UFUNCTION(BlueprintImplementableEvent, Category="LS/Interact|Marker")
	void OnDistanceRatioUpdated(float DistanceRatio);
};
