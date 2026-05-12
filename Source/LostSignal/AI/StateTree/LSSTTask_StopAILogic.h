#pragma once

#include "CoreMinimal.h"
#include "Tasks/StateTreeAITask.h"
#include "LSSTTask_StopAILogic.generated.h"

/**
 * StateTree task for terminal AI shutdown.
 * Use this in a Dead state after death has been represented by GAS/AnimBP state.
 */
USTRUCT()
struct FLSSTTask_StopAILogicInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="LS/AI")
	FString Reason = TEXT("Dead State");
};

/** Stops the owning AIController brain so the Dead StateTree state becomes terminal. */
USTRUCT(meta=(DisplayName="LS Stop AI Logic", Category="LS|AI|Action"))
struct LOSTSIGNAL_API FLSSTTask_StopAILogic : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLSSTTask_StopAILogicInstanceData;

	FLSSTTask_StopAILogic();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
