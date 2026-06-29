#include "AI/StateTree/LSSTCondition_IsDormantByDistance.h"

#include "StateTreeExecutionContext.h"

bool FLSSTCondition_IsDormantByDistance::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	return InstanceData.bInvert ? !InstanceData.bIsDormantByDistance : InstanceData.bIsDormantByDistance;
}
