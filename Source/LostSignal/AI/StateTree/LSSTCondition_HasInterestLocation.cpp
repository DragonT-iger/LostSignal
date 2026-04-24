#include "AI/StateTree/LSSTCondition_HasInterestLocation.h"

#include "StateTreeExecutionContext.h"

bool FLSSTCondition_HasInterestLocation::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	return InstanceData.bInvert ? !InstanceData.bHasInterestLocation : InstanceData.bHasInterestLocation;
}
