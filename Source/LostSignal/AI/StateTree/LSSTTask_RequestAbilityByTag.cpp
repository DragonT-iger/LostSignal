#include "AI/StateTree/LSSTTask_RequestAbilityByTag.h"

#include "AI/LSMonsterCombatComponent.h"
#include "Characters/LSEnemyCharacter.h"
#include "StateTreeExecutionContext.h"

FLSSTTask_RequestAbilityByTag::FLSSTTask_RequestAbilityByTag()
{
	bShouldCallTick = true;
	bShouldCopyBoundPropertiesOnTick = true;
}

EStateTreeRunStatus FLSSTTask_RequestAbilityByTag::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
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

	FGameplayTag AbilityTag = InstanceData.AbilityTag;
	if (!AbilityTag.IsValid() && InstanceData.bFallbackToDefaultAttackTag)
	{
		AbilityTag = InstanceData.CombatComponent->GetDefaultAttackAbilityTag();
	}

	InstanceData.ActiveAbilityTag = AbilityTag;
	return InstanceData.CombatComponent->RequestAbilityByTag(AbilityTag)
		? EStateTreeRunStatus::Running
		: EStateTreeRunStatus::Failed;
}

EStateTreeRunStatus FLSSTTask_RequestAbilityByTag::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (!InstanceData.CombatComponent)
	{
		return EStateTreeRunStatus::Failed;
	}

	if (InstanceData.bCancelAbilityWhenTargetLeavesRange && InstanceData.EnemyCharacter && InstanceData.TargetActor && InstanceData.CancelIfTargetFartherThan > 0.0f)
	{
		const float DistanceToTarget = FVector::Dist2D(
			InstanceData.EnemyCharacter->GetActorLocation(),
			InstanceData.TargetActor->GetActorLocation());

		if (DistanceToTarget > InstanceData.CancelIfTargetFartherThan)
		{
			InstanceData.CombatComponent->CancelAbilityByTag(InstanceData.ActiveAbilityTag);
			return EStateTreeRunStatus::Succeeded;
		}
	}

	return InstanceData.CombatComponent->IsAbilityActiveByTag(InstanceData.ActiveAbilityTag)
		? EStateTreeRunStatus::Running
		: EStateTreeRunStatus::Succeeded;
}
