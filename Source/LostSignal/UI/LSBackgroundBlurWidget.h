// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LSBackgroundBlurWidget.generated.h"

// 기존 WBP_BackgroundBlur 에셋의 네이티브 부모 호환용 클래스.
// 런타임에서는 생성하지 않으며, 배경 효과는 각 패널 WBP가 직접 소유한다.
UCLASS(BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSBackgroundBlurWidget : public UUserWidget
{
	GENERATED_BODY()
};
