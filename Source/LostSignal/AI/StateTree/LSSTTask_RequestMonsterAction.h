#pragma once

#include "CoreMinimal.h"
#include "Tasks/StateTreeAITask.h"
#include "LSSTTask_RequestMonsterAction.generated.h"

class ALSEnemyCharacter;
class ULSMonsterCombatComponent;

/** Input payload for requesting a data-driven monster action (DT_MonsterAction) from StateTree. */
USTRUCT()
struct FLSSTTask_RequestMonsterActionInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="LS/AI")
	TObjectPtr<ALSEnemyCharacter> EnemyCharacter = nullptr;

	UPROPERTY(EditAnywhere, Category="LS/AI")
	TObjectPtr<ULSMonsterCombatComponent> CombatComponent = nullptr;

	UPROPERTY(EditAnywhere, Category="LS/AI")
	TObjectPtr<AActor> TargetActor = nullptr;

	// 공격 모션은 기본적으로 캔슬하지 않는다(기획 예외). 켜면 타겟이 멀어질 때 진행 중 액션을 취소.
	UPROPERTY(EditAnywhere, Category="LS/AI")
	bool bCancelActionWhenTargetLeavesRange = false;

	UPROPERTY(EditAnywhere, Category="LS/AI", meta=(ClampMin="0.0"))
	float CancelIfTargetFartherThan = 600.0f;

	/** 공격 발동 전 타겟 방향으로 회전하는 시간(초). 0이면 즉시 조준 후 바로 공격. */
	UPROPERTY(EditAnywhere, Category="LS/AI", meta=(ClampMin="0.0"))
	float AttackAlignDuration = 0.2f;

	UPROPERTY(Transient, VisibleAnywhere, Category="LS/AI")
	float AlignElapsed = 0.0f;

	UPROPERTY(Transient, VisibleAnywhere, Category="LS/AI")
	float AlignStartYaw = 0.0f;

	UPROPERTY(Transient, VisibleAnywhere, Category="LS/AI")
	bool bActionRequested = false;
};

/** StateTree task that asks the monster combat component to select and run an action for the target. */
USTRUCT(meta=(DisplayName="LS Request Monster Action", Category="LS|AI|Action"))
struct LOSTSIGNAL_API FLSSTTask_RequestMonsterAction : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLSSTTask_RequestMonsterActionInstanceData;

	FLSSTTask_RequestMonsterAction();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};
