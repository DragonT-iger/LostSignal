#include "AI/StateTree/LSSTTask_ResetMonsterCombat.h"

#include "AbilitySystemComponent.h"
#include "AI/LSMonsterCombatComponent.h"
#include "AIController.h"
#include "Characters/LSEnemyCharacter.h"
#include "GAS/Effects/LSGE_FullHeal.h"
#include "StateTreeExecutionContext.h"
#include "LostSignal.h"

FLSSTTask_ResetMonsterCombat::FLSSTTask_ResetMonsterCombat()
{
	bShouldCallTick = false;
	bShouldCopyBoundPropertiesOnTick = false;

#if WITH_EDITORONLY_DATA
	bConsideredForCompletion = false;
#endif
}

EStateTreeRunStatus FLSSTTask_ResetMonsterCombat::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	// 에셋 바인딩 누락 대비 컨텍스트 소유자에서 해석.
	if (!InstanceData.EnemyCharacter)
	{
		if (const AAIController* AIController = Cast<AAIController>(Context.GetOwner()))
		{
			InstanceData.EnemyCharacter = Cast<ALSEnemyCharacter>(AIController->GetPawn());
		}
	}

	// 리셋 실패가 복귀 자체를 막지 않도록 실패 시에도 Running을 유지한다.
	if (!InstanceData.EnemyCharacter)
	{
		UE_LOG(LogLS, Warning, TEXT("LSSTTask_ResetMonsterCombat: EnemyCharacter is invalid, monster combat reset skipped."));
		return EStateTreeRunStatus::Running;
	}

	// 복귀 시작 즉시 풀피 — "깎아놓고 도망 -> 재진입" 누적 딜을 차단한다.
	if (UAbilitySystemComponent* ASC = InstanceData.EnemyCharacter->GetAbilitySystemComponent())
	{
		ASC->ApplyGameplayEffectToSelf(GetDefault<ULSGE_FullHeal>(), 1.0f, ASC->MakeEffectContext());
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("%s: missing ASC, monster health reset skipped."), *GetNameSafe(InstanceData.EnemyCharacter));
	}

	if (ULSMonsterCombatComponent* CombatComponent = InstanceData.EnemyCharacter->FindComponentByClass<ULSMonsterCombatComponent>())
	{
		CombatComponent->ResetActionCooldowns();
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("%s: missing CombatComponent, monster cooldown reset skipped."), *GetNameSafe(InstanceData.EnemyCharacter));
	}

	return EStateTreeRunStatus::Running;
}
