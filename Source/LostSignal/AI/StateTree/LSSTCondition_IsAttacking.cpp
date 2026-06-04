#include "AI/StateTree/LSSTCondition_IsAttacking.h"

#include "StateTreeExecutionContext.h"

bool FLSSTCondition_IsAttacking::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	return InstanceData.bInvert ? !InstanceData.bIsAttacking : InstanceData.bIsAttacking;
}
