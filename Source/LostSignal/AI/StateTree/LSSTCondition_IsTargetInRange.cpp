#include "AI/StateTree/LSSTCondition_IsTargetInRange.h"

#include "StateTreeExecutionContext.h"

bool FLSSTCondition_IsTargetInRange::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	const bool bIsInRange = InstanceData.DistanceToTarget <= InstanceData.RequiredRange;
	return InstanceData.bInvert ? !bIsInRange : bIsInRange;
}
