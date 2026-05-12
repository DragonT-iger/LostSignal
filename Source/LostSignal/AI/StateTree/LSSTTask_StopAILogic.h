#pragma once

#include "CoreMinimal.h"
#include "Tasks/StateTreeAITask.h"
#include "LSSTTask_StopAILogic.generated.h"

class AAIController;

/** Input payload for explicitly stopping AI brain logic from a StateTree state such as Dead. */
USTRUCT()
struct FLSSTTask_StopAILogicInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="LS/AI")
	TObjectPtr<AAIController> AIController = nullptr;

	/** Debug reason forwarded to BrainComponent->StopLogic. */
	UPROPERTY(EditAnywhere, Category="LS/AI")
	FString Reason = TEXT("StateTree Stop");
};

/** StateTree task that cleanly stops the owning AI brain when entering a terminal state. */
USTRUCT(meta=(DisplayName="LS Stop AI Logic", Category="LS|AI|Action"))
struct LOSTSIGNAL_API FLSSTTask_StopAILogic : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLSSTTask_StopAILogicInstanceData;

	FLSSTTask_StopAILogic();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
