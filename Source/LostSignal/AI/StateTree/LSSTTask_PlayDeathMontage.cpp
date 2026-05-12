#include "AI/StateTree/LSSTTask_PlayDeathMontage.h"

#include "Animation/AnimMontage.h"
#include "Characters/LSEnemyCharacter.h"
#include "LostSignal.h"
#include "StateTreeExecutionContext.h"

FLSSTTask_PlayDeathMontage::FLSSTTask_PlayDeathMontage()
{
	bShouldCallTick = false;
	bShouldCopyBoundPropertiesOnTick = false;
}

EStateTreeRunStatus FLSSTTask_PlayDeathMontage::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);
	if (!InstanceData.EnemyCharacter)
	{
		return EStateTreeRunStatus::Failed;
	}

	UAnimMontage* DeathMontage = InstanceData.EnemyCharacter->GetDeathMontage();
	if (!DeathMontage)
	{
		UE_LOG(LogLS, Warning, TEXT("%s has no death montage assigned."), *GetNameSafe(InstanceData.EnemyCharacter));
		return EStateTreeRunStatus::Failed;
	}

	const float PlayedDuration = InstanceData.EnemyCharacter->PlayAnimMontage(DeathMontage);
	if (PlayedDuration <= 0.0f)
	{
		UE_LOG(LogLS, Warning, TEXT("%s failed to play death montage %s."), *GetNameSafe(InstanceData.EnemyCharacter), *GetNameSafe(DeathMontage));
		return EStateTreeRunStatus::Failed;
	}

	UE_LOG(LogLS, Log, TEXT("%s started death montage %s."), *GetNameSafe(InstanceData.EnemyCharacter), *GetNameSafe(DeathMontage));
	return EStateTreeRunStatus::Succeeded;
}
