#include "AI/StateTree/LSSTEvaluator_MonsterSense.h"

#include "AI/LSMonsterCombatComponent.h"
#include "AI/LSMonsterSenseComponent.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Characters/LSEnemyCharacter.h"
#include "GAS/LSGameplayTags.h"
#include "StateTreeExecutionContext.h"
#include "LostSignal.h"

void FLSSTEvaluator_MonsterSense::TreeStart(FStateTreeExecutionContext& Context) const
{
	UE_LOG(LogLS, Warning, TEXT("Tree Start"));
	UpdateData(Context);
}

void FLSSTEvaluator_MonsterSense::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	UpdateData(Context);
}

void FLSSTEvaluator_MonsterSense::UpdateData(FStateTreeExecutionContext& Context) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	if (!InstanceData.EnemyCharacter && InstanceData.AIController)
	{
		InstanceData.EnemyCharacter = Cast<ALSEnemyCharacter>(InstanceData.AIController->GetPawn());
	}

	if (!InstanceData.SenseComponent && InstanceData.EnemyCharacter)
	{
		InstanceData.SenseComponent = InstanceData.EnemyCharacter->FindComponentByClass<ULSMonsterSenseComponent>();
	}

	if (!InstanceData.CombatComponent && InstanceData.EnemyCharacter)
	{
		InstanceData.CombatComponent = InstanceData.EnemyCharacter->FindComponentByClass<ULSMonsterCombatComponent>();
	}

	if (!InstanceData.SenseComponent)
	{
		InstanceData.CurrentTarget = nullptr;
		InstanceData.bHasVisualTarget = false;
		InstanceData.bHasInterestLocation = false;
		InstanceData.DistanceToTarget = 0.0f;
		InstanceData.bIsDead = false;
		return;
	}

	InstanceData.CurrentTarget = InstanceData.SenseComponent->GetCurrentTarget();
	InstanceData.InterestLocation = InstanceData.SenseComponent->GetInterestLocation();
	InstanceData.HomeLocation = InstanceData.SenseComponent->GetHomeLocation();
	InstanceData.LeashDistance = InstanceData.SenseComponent->GetLeashDistance();
	InstanceData.AlertDuration = InstanceData.SenseComponent->GetAlertDuration();
	InstanceData.AlertMoveSpeedMultiplier = InstanceData.SenseComponent->GetAlertMoveSpeedMultiplier();
	InstanceData.bHasVisualTarget = InstanceData.SenseComponent->HasVisualTarget();
	InstanceData.bHasInterestLocation = InstanceData.SenseComponent->HasInterestLocation();
	InstanceData.bIsDead = false;

	if (InstanceData.EnemyCharacter)
	{
		if (UAbilitySystemComponent* ASC = InstanceData.EnemyCharacter->GetAbilitySystemComponent())
		{
			// StateTree does not decide death itself; it only mirrors the final GAS state tag for transitions.
			InstanceData.bIsDead = ASC->HasMatchingGameplayTag(LSGameplayTags::State_Dead);
		}
	}

	if (InstanceData.EnemyCharacter && InstanceData.CurrentTarget)
	{
		InstanceData.DistanceToTarget = FVector::Dist2D(
			InstanceData.EnemyCharacter->GetActorLocation(),
			InstanceData.CurrentTarget->GetActorLocation()
		);
	}
	else
	{
		InstanceData.DistanceToTarget = 0.0f;
	}
}
