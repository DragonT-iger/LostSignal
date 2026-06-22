// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LSModalBackdropWidget.generated.h"

class UBackgroundBlur;

// 인벤토리/창고/칩스테이션/루트드랍 같은 모달 패널 뒤에 깔리는 공유 블러 백드롭.
// 표시/숨김은 ALSPlayerControllerBase::UpdateModalBackdropVisibility가 제어한다(이 위젯엔 로직 없음).
// Z-order 규칙은 LSUILayer.h 참고. 입력은 위 패널이 받도록 표시 시 HitTestInvisible로 둔다.
UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSModalBackdropWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

protected:
	// 풀스크린 블러. WBP가 반드시 이 이름의 BackgroundBlur를 포함해야 한다(BindWidget 강제).
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<UBackgroundBlur> BackdropBlur;
};
