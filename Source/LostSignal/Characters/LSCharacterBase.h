// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LSCharacterBase.generated.h"

/**
 * Abstract base character for LostSignal.
 * 
 ACharacter 
└── ALSCharacter [abstract] ← GAS IAbilitySystemInterface, 공통 로직
├── ALSPlayerCharacter [abstract] ← 카메라, 마우스 추적, Enhanced Input
│ └── BP_PlayerCharacter ← (메시, 에셋 매핑만)
└── ALSEnemyCharacter [abstract] ← 카메라 없음, AI용 Move/Look 오버라이드
  └── BP_Enemy_Base ← (메시, 에셋 매핑만)
 * 
 */
UCLASS(Abstract)
class ALSCharacterBase : public ACharacter
{
	GENERATED_BODY()

public:

	ALSCharacterBase();
};
