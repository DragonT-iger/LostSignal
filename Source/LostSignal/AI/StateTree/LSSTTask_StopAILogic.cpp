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
	if (!InstanceData.AIController)
	{
		return EStateTreeRunStatus::Failed;
	}

	if (InstanceData.AIController->BrainComponent)
	{
		UE_LOG(
			LogLS,
			Log,
			TEXT("%s StopLogic from StateTree. Reason=%s"),
			*GetNameSafe(InstanceData.AIController),
			*InstanceData.Reason
		);
		InstanceData.AIController->BrainComponent->StopLogic(InstanceData.Reason);
	}

	return EStateTreeRunStatus::Succeeded;
}
