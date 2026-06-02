#include "AI/StateTree/LSSTCondition_IsBeyondLeashDistance.h"

#include "StateTreeExecutionContext.h"

bool FLSSTCondition_IsBeyondLeashDistance::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	return InstanceData.bInvert ? !InstanceData.bIsBeyondLeashDistance : InstanceData.bIsBeyondLeashDistance;
}
