#include "AI/StateTree/LSSTTask_RequestMonsterAction.h"

#include "AI/LSMonsterCombatComponent.h"
#include "Characters/LSEnemyCharacter.h"
#include "GAS/LSGameplayTags.h"
#include "StateTreeExecutionContext.h"

FLSSTTask_RequestMonsterAction::FLSSTTask_RequestMonsterAction()
{
	bShouldCallTick = true;
	bShouldCopyBoundPropertiesOnTick = true;
}

EStateTreeRunStatus FLSSTTask_RequestMonsterAction::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (!InstanceData.CombatComponent && InstanceData.EnemyCharacter)
	{
		InstanceData.CombatComponent = InstanceData.EnemyCharacter->FindComponentByClass<ULSMonsterCombatComponent>();
	}

	if (!InstanceData.CombatComponent)
	{
		return EStateTreeRunStatus::Failed;
	}

	// 어떤 액션을 할지는 CombatComponent가 거리/쿨다운으로 선택한다.
	return InstanceData.CombatComponent->RequestAction(InstanceData.TargetActor)
		? EStateTreeRunStatus::Running
		: EStateTreeRunStatus::Failed;
}

EStateTreeRunStatus FLSSTTask_RequestMonsterAction::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (!InstanceData.CombatComponent)
	{
		return EStateTreeRunStatus::Failed;
	}

	if (InstanceData.CombatComponent->IsAbilityActiveByTag(LSGameplayTags::Ability_MonsterAction))
	{
		return EStateTreeRunStatus::Running;
	}

	if (InstanceData.bCancelActionWhenTargetLeavesRange && InstanceData.EnemyCharacter && InstanceData.TargetActor && InstanceData.CancelIfTargetFartherThan > 0.0f)
	{
		const float DistanceToTarget = FVector::Dist2D(
			InstanceData.EnemyCharacter->GetActorLocation(),
			InstanceData.TargetActor->GetActorLocation());

		if (DistanceToTarget > InstanceData.CancelIfTargetFartherThan)
		{
			InstanceData.CombatComponent->CancelAbilityByTag(LSGameplayTags::Ability_MonsterAction);
			return EStateTreeRunStatus::Succeeded;
		}
	}

	return EStateTreeRunStatus::Succeeded;
}
