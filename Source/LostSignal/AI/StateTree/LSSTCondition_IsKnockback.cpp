#include "AI/StateTree/LSSTCondition_IsKnockback.h"

#include "StateTreeExecutionContext.h"

bool FLSSTCondition_IsKnockback::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	return InstanceData.bInvert ? !InstanceData.bIsKnockback : InstanceData.bIsKnockback;
}
