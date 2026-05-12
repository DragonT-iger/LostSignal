#include "AI/StateTree/LSSTTask_StopAILogic.h"

#include "AIController.h"
#include "BrainComponent.h"
#include "LostSignal.h"
#include "StateTreeExecutionContext.h"

FLSSTTask_StopAILogic::FLSSTTask_StopAILogic()
{
	bShouldCallTick = false;
	bShouldCopyBoundPropertiesOnTick = false;
}

EStateTreeRunStatus FLSSTTask_StopAILogic::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	AAIController* AIController = Cast<AAIController>(Context.GetOwner());
	if (!AIController || !AIController->HasAuthority())
	{
		return EStateTreeRunStatus::Failed;
	}

	if (!AIController->BrainComponent)
	{
		UE_LOG(LogLS, Warning, TEXT("%s StopAILogic task skipped because BrainComponent is missing."), *GetNameSafe(AIController));
		return EStateTreeRunStatus::Failed;
	}

	UE_LOG(LogLS, Log, TEXT("%s StopLogic from StateTree task. Reason=%s"), *GetNameSafe(AIController), *InstanceData.Reason);
	AIController->BrainComponent->StopLogic(InstanceData.Reason);
	return EStateTreeRunStatus::Succeeded;
}
