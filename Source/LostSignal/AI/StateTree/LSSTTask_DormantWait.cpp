#include "AI/StateTree/LSSTTask_DormantWait.h"

#include "AIController.h"
#include "StateTreeExecutionContext.h"

EStateTreeRunStatus FLSSTTask_DormantWait::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (!InstanceData.AIController)
	{
		InstanceData.AIController = Cast<AAIController>(Context.GetOwner());
	}

	if (InstanceData.AIController)
	{
		InstanceData.AIController->StopMovement();
		InstanceData.AIController->ClearFocus(EAIFocusPriority::Gameplay);
	}

	return EStateTreeRunStatus::Running;
}
