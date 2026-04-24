#include "AI/StateTree/LSSTCondition_HasVisualTarget.h"

#include "StateTreeExecutionContext.h"

bool FLSSTCondition_HasVisualTarget::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	return InstanceData.bInvert ? !InstanceData.bHasVisualTarget : InstanceData.bHasVisualTarget;
}
