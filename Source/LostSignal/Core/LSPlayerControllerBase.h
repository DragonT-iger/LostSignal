// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LSPlayerControllerBase.generated.h"

class UInputMappingContext;

/**
 * Base PlayerController for LostSignal.
 * 마우스 커서 표시, GameAndUI 입력 모드, IMC 등록을 담당.
 * 실제 IMC 에셋은 파생 Blueprint에서 DefaultMappingContexts 배열에 할당.
 */
UCLASS(Abstract)
class ALSPlayerControllerBase : public APlayerController
{
	GENERATED_BODY()

protected:

	/** 게임플레이에 사용할 Input Mapping Context 목록. 우선순위 0으로 일괄 등록됨. */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<TObjectPtr<UInputMappingContext>> DefaultMappingContexts;

protected:

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
};
