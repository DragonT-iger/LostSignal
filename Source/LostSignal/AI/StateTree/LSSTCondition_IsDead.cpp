#include "AI/StateTree/LSSTCondition_IsDead.h"

#include "StateTreeExecutionContext.h"

bool FLSSTCondition_IsDead::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	return InstanceData.bInvert ? !InstanceData.bIsDead : InstanceData.bIsDead;
}
