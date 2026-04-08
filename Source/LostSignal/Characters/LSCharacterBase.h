// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LSCharacterBase.generated.h"

/**
 * Abstract base character for LostSignal.
 * 공통 이동 세팅만 담당. 카메라는 ALSPlayerCharacter에만 있음.
 *
 * Unity 비교: abstract class BaseCharacter : MonoBehaviour에 해당.
 */
UCLASS(Abstract)
class ALSCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:

	ALSCharacterBase();
};
