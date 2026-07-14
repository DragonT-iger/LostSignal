// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Characters/Enemys/LSEnemyCharacter.h"
#include "LSEnemyAndroid.generated.h"

/**
 * 안드로이드 몬스터의 데이터 주도 구체 클래스.
 * DT_MonsterStat의 "10006" row를 사용하며, 에셋 매핑은 이 클래스를 상속한 BP에서 수행한다.
 */
UCLASS()
class LOSTSIGNAL_API ALSEnemyAndroid : public ALSEnemyCharacter
{
	GENERATED_BODY()

public:
	ALSEnemyAndroid();
};
