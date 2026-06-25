#include "AI/StateTree/LSSTCondition_HasUsableAction.h"

#include "StateTreeExecutionContext.h"

bool FLSSTCondition_HasUsableAction::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	return InstanceData.bInvert ? !InstanceData.bHasUsableAction : InstanceData.bHasUsableAction;
}
