#pragma once

#include "CoreMinimal.h"
#include "Tasks/StateTreeAITask.h"
#include "LSSTTask_ApplyMoveSpeedMultiplier.generated.h"

class AAIController;
class ALSEnemyCharacter;

/** Input payload for applying a movement-speed multiplier while a state is active. */
USTRUCT()
struct FLSSTTask_ApplyMoveSpeedMultiplierInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="LS/AI")
	TObjectPtr<AAIController> AIController = nullptr;

	UPROPERTY(EditAnywhere, Category="LS/AI")
	TObjectPtr<ALSEnemyCharacter> EnemyCharacter = nullptr;

	/** 적용할 이동 속도 배수(base MaxWalkSpeed × 배수). evaluator의 AlertMoveSpeedMultiplier에 바인딩. 미바인딩 시 1.0(=base 풀 스피드). */
	UPROPERTY(EditAnywhere, Category="LS/AI", meta=(ClampMin="0.0"))
	float MoveSpeedMultiplier = 1.0f;
};

/** StateTree task: 상태 진입 시 MaxWalkSpeed에 배수를 적용하고 이탈 시 복원한다. 기본 MoveTo를 쓰는 상태(Chase 등)에 병렬로 붙인다. */
USTRUCT(meta=(DisplayName="LS Apply Move Speed Multiplier", Category="LS|AI|Action"))
struct LOSTSIGNAL_API FLSSTTask_ApplyMoveSpeedMultiplier : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLSSTTask_ApplyMoveSpeedMultiplierInstanceData;

	FLSSTTask_ApplyMoveSpeedMultiplier();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
