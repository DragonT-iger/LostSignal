#include "AI/StateTree/LSSTTask_RequestMonsterAction.h"

#include "AI/LSMonsterCombatComponent.h"
#include "Characters/LSEnemyCharacter.h"
#include "GAS/LSGameplayTags.h"
#include "StateTreeExecutionContext.h"

namespace
{
	// 공격 발동 전 회전: 시작 yaw에서 타겟 방향 yaw로 AttackAlignDuration에 걸쳐 보간. 완료 시 true.
	bool AdvanceMonsterAttackAlignYaw(FLSSTTask_RequestMonsterActionInstanceData& InstanceData, float DeltaTime)
	{
		ALSEnemyCharacter* EnemyCharacter = InstanceData.EnemyCharacter;
		const AActor* Target = InstanceData.TargetActor;
		if (!EnemyCharacter || !Target)
		{
			return true;
		}

		InstanceData.AlignElapsed += DeltaTime;
		const float Alpha = FMath::Clamp(InstanceData.AlignElapsed / InstanceData.AttackAlignDuration, 0.0f, 1.0f);

		FVector ToTarget = Target->GetActorLocation() - EnemyCharacter->GetActorLocation();
		ToTarget.Z = 0.0f;
		if (!ToTarget.IsNearlyZero())
		{
			// 타겟이 움직여도 매 틱 현재 방향을 목표로 다시 보간(마지막에 정확히 타겟을 향함).
			const float DesiredYaw = ToTarget.Rotation().Yaw;
			const float DeltaYaw = FMath::FindDeltaAngleDegrees(InstanceData.AlignStartYaw, DesiredYaw);
			FRotator NewRotation = EnemyCharacter->GetActorRotation();
			NewRotation.Yaw = InstanceData.AlignStartYaw + DeltaYaw * Alpha;
			EnemyCharacter->SetActorRotation(NewRotation);
		}

		return Alpha >= 1.0f;
	}
}

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

	InstanceData.AlignElapsed = 0.0f;
	InstanceData.bActionRequested = false;

	// 회전 시간이 없거나 회전할 대상이 없으면 기존처럼 즉시 발동.
	if (InstanceData.AttackAlignDuration <= 0.0f || !InstanceData.TargetActor || !InstanceData.EnemyCharacter)
	{
		InstanceData.bActionRequested = true;
		return InstanceData.CombatComponent->RequestAction(InstanceData.TargetActor)
			? EStateTreeRunStatus::Running
			: EStateTreeRunStatus::Failed;
	}

	// 회전 단계 시작 — 발동은 Tick에서 회전이 끝난 뒤.
	InstanceData.AlignStartYaw = InstanceData.EnemyCharacter->GetActorRotation().Yaw;
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FLSSTTask_RequestMonsterAction::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (!InstanceData.CombatComponent)
	{
		return EStateTreeRunStatus::Failed;
	}

	// 회전 단계: 완료 전까지 발동을 미룬다.
	if (!InstanceData.bActionRequested)
	{
		if (!AdvanceMonsterAttackAlignYaw(InstanceData, DeltaTime))
		{
			return EStateTreeRunStatus::Running;
		}

		InstanceData.bActionRequested = true;
		// 어떤 액션을 할지는 CombatComponent가 거리/쿨다운으로 선택한다.
		return InstanceData.CombatComponent->RequestAction(InstanceData.TargetActor)
			? EStateTreeRunStatus::Running
			: EStateTreeRunStatus::Failed;
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
