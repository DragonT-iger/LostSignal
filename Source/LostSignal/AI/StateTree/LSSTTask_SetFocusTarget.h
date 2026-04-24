#pragma once

#include "CoreMinimal.h"
#include "Tasks/StateTreeAITask.h"
#include "LSSTTask_SetFocusTarget.generated.h"

class AAIController;

/** Input payload for setting or clearing AI focus on a target actor. */
USTRUCT()
struct FLSSTTask_SetFocusTargetInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Context")
	TObjectPtr<AAIController> AIController = nullptr;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(EditAnywhere, Category="Task")
	bool bClearFocusOnExit = true;
};

/** StateTree task that keeps controller focus aligned with the current target. */
USTRUCT(meta=(DisplayName="LS Set Focus Target", Category="LS|AI|Action"))
struct LOSTSIGNAL_API FLSSTTask_SetFocusTarget : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLSSTTask_SetFocusTargetInstanceData;

	FLSSTTask_SetFocusTarget();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
