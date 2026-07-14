// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Characters/Enemys/LSEnemyCharacter.h"
#include "LSEnemyHyena.generated.h"

/**
 * 몬스터1: 사족보행 근접형(Hyena_A2).
 * 데이터 주도 설계를 따르는 얇은 구체 클래스 — DT_MonsterStat의 "10001" row만 가리킨다.
 * 메시/몽타주/어빌리티/DataTable 에셋 매핑은 이 클래스를 상속한 BP에서 수행한다.
 */
UCLASS()
class LOSTSIGNAL_API ALSEnemyHyena : public ALSEnemyCharacter
{
	GENERATED_BODY()

public:
	ALSEnemyHyena();
};
