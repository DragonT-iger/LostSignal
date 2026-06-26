#include "AI/StateTree/LSSTTask_WaitForKnockbackEnd.h"

#include "AI/LSMonsterCombatComponent.h"
#include "AIController.h"
#include "Characters/LSEnemyCharacter.h"
#include "GAS/LSGameplayTags.h"
#include "StateTreeExecutionContext.h"

FLSSTTask_WaitForKnockbackEnd::FLSSTTask_WaitForKnockbackEnd()
{
	bShouldCallTick = true;
	bShouldCopyBoundPropertiesOnTick = true;
}

EStateTreeRunStatus FLSSTTask_WaitForKnockbackEnd::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	// 에셋 AIController 바인딩 누락 대비 컨텍스트 소유자에서 해석.
	if (!InstanceData.AIController)
	{
		InstanceData.AIController = Cast<AAIController>(Context.GetOwner());
	}

	if (!InstanceData.EnemyCharacter && InstanceData.AIController)
	{
		InstanceData.EnemyCharacter = Cast<ALSEnemyCharacter>(InstanceData.AIController->GetPawn());
	}

	if (!InstanceData.CombatComponent && InstanceData.EnemyCharacter)
	{
		InstanceData.CombatComponent = InstanceData.EnemyCharacter->FindComponentByClass<ULSMonsterCombatComponent>();
	}

	if (InstanceData.AIController)
	{
		InstanceData.AIController->StopMovement();
	}

	if (InstanceData.CombatComponent)
	{
		FGameplayTag AbilityTag = InstanceData.AttackAbilityTag;
		if (!AbilityTag.IsValid() && InstanceData.bFallbackToDefaultAttackTag)
		{
			AbilityTag = LSGameplayTags::Ability_MonsterAction;
		}

		InstanceData.CombatComponent->CancelAbilityByTag(AbilityTag);
	}

	return InstanceData.bIsKnockback ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}

EStateTreeRunStatus FLSSTTask_WaitForKnockbackEnd::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	return InstanceData.bIsKnockback ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}
