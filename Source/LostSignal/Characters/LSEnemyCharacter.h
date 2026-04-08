// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Characters/LSCharacterBase.h"
#include "LSEnemyCharacter.generated.h"

/**
 * Abstract enemy character for LostSignal.
 * 카메라 없음. AI Controller(BehaviorTree)가 이동/회전 제어.
 * 구체적인 적 타입은 이 클래스를 상속해 BP에서 메시·에셋 할당.
 */
UCLASS(Abstract)
class ALSEnemyCharacter : public ALSCharacterBase
{
	GENERATED_BODY()

public:

	ALSEnemyCharacter();
};
