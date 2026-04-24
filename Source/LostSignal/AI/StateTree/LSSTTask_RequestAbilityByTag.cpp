#include "AI/StateTree/LSSTTask_RequestAbilityByTag.h"

#include "AI/LSMonsterCombatComponent.h"
#include "Characters/LSEnemyCharacter.h"
#include "StateTreeExecutionContext.h"

FLSSTTask_RequestAbilityByTag::FLSSTTask_RequestAbilityByTag()
{
	bShouldCallTick = false;
	bShouldCopyBoundPropertiesOnTick = false;
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

	return InstanceData.CombatComponent->RequestAbilityByTag(AbilityTag)
		? EStateTreeRunStatus::Succeeded
		: EStateTreeRunStatus::Failed;
}
