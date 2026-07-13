#pragma once

#include "CoreMinimal.h"
#include "Tasks/StateTreeAITask.h"
#include "LSSTTask_ResetMonsterCombat.generated.h"

class ALSEnemyCharacter;

/** Input payload for resetting monster combat state on return home. */
USTRUCT()
struct FLSSTTask_ResetMonsterCombatInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="LS/AI")
	TObjectPtr<ALSEnemyCharacter> EnemyCharacter = nullptr;
};

/** 복귀(ReturnHome) 진입 시 체력 전량 회복 + 액션 쿨다운 초기화. 복귀 태스크와 병렬 배치한다. */
USTRUCT(meta=(DisplayName="LS Reset Monster Combat", Category="LS|AI|Action"))
struct LOSTSIGNAL_API FLSSTTask_ResetMonsterCombat : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLSSTTask_ResetMonsterCombatInstanceData;

	FLSSTTask_ResetMonsterCombat();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
