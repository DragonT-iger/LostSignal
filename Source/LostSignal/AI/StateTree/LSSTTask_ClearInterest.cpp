#include "AI/StateTree/LSSTTask_ClearInterest.h"

#include "AI/LSMonsterSenseComponent.h"
#include "Characters/Enemys/LSEnemyCharacter.h"
#include "StateTreeExecutionContext.h"

FLSSTTask_ClearInterest::FLSSTTask_ClearInterest()
{
	bShouldCallTick = false;
	bShouldCopyBoundPropertiesOnTick = false;
}

EStateTreeRunStatus FLSSTTask_ClearInterest::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (!InstanceData.SenseComponent && InstanceData.EnemyCharacter)
	{
		InstanceData.SenseComponent = InstanceData.EnemyCharacter->FindComponentByClass<ULSMonsterSenseComponent>();
	}

	if (!InstanceData.SenseComponent)
	{
		return EStateTreeRunStatus::Failed;
	}

	InstanceData.SenseComponent->ClearInterest();
	return EStateTreeRunStatus::Succeeded;
}
