#include "AI/StateTree/LSSTTask_SetFocusTarget.h"

#include "AIController.h"
#include "StateTreeExecutionContext.h"

FLSSTTask_SetFocusTarget::FLSSTTask_SetFocusTarget()
{
	bShouldCallTick = false;
	bShouldCopyBoundPropertiesOnTick = false;

#if WITH_EDITORONLY_DATA
	bConsideredForCompletion = false;
#endif
}

EStateTreeRunStatus FLSSTTask_SetFocusTarget::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (!InstanceData.AIController)
	{
		return EStateTreeRunStatus::Failed;
	}

	if (InstanceData.TargetActor)
	{
		InstanceData.AIController->SetFocus(InstanceData.TargetActor);
	}
	else
	{
		InstanceData.AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}

	return EStateTreeRunStatus::Running;
}

void FLSSTTask_SetFocusTarget::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (InstanceData.AIController && InstanceData.bClearFocusOnExit)
	{
		InstanceData.AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}
}
