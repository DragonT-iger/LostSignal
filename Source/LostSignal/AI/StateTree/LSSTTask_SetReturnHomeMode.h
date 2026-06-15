#pragma once

#include "CoreMinimal.h"
#include "Tasks/StateTreeAITask.h"
#include "LSSTTask_SetReturnHomeMode.generated.h"

class AAIController;
class ALSEnemyCharacter;
class ULSMonsterSenseComponent;

/** Input payload for applying return-home movement and vision overrides. */
USTRUCT()
struct FLSSTTask_SetReturnHomeModeInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="LS/AI")
	TObjectPtr<AAIController> AIController = nullptr;

	UPROPERTY(EditAnywhere, Category="LS/AI")
	TObjectPtr<ALSEnemyCharacter> EnemyCharacter = nullptr;

	UPROPERTY(EditAnywhere, Category="LS/AI")
	TObjectPtr<ULSMonsterSenseComponent> SenseComponent = nullptr;

	UPROPERTY(EditAnywhere, Category="LS/AI", meta=(ClampMin="0.0"))
	float AlertMoveSpeedMultiplier = 1.2f;

	UPROPERTY(EditAnywhere, Category="LS/AI")
	bool bClearFocusOnEnter = true;

	UPROPERTY(EditAnywhere, Category="LS/AI")
	bool bClearStaleInterestOnEnter = true;

	UPROPERTY(Transient, VisibleAnywhere, Category="LS/AI")
	float PreviousMaxWalkSpeed = 0.0f;
};

/** StateTree task for ReturnHome: faster return movement and max sight radius until the state exits. */
USTRUCT(meta=(DisplayName="LS Set Return Home Mode", Category="LS|AI|Action"))
struct LOSTSIGNAL_API FLSSTTask_SetReturnHomeMode : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLSSTTask_SetReturnHomeModeInstanceData;

	FLSSTTask_SetReturnHomeMode();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
