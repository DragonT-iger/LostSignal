#include "GAS/Abilities/LSGA_MonsterAction.h"

#include "AI/LSMonsterCombatComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Characters/LSEnemyCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/LSMonsterActionRow.h"
#include "GAS/LSGameplayTags.h"
#include "LostSignal.h"

ULSGA_MonsterAction::ULSGA_MonsterAction()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(LSGameplayTags::Ability_MonsterAction);
	SetAssetTags(AssetTags);

	ActivationOwnedTags.AddTag(LSGameplayTags::Combat_Attacking);
	ActivationBlockedTags.AddTag(LSGameplayTags::Combat_Attacking);
	ActivationBlockedTags.AddTag(LSGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(LSGameplayTags::State_Stunned);

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void ULSGA_MonsterAction::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	bEndingAbility = false;

	ALSEnemyCharacter* EnemyCharacter = Cast<ALSEnemyCharacter>(GetAvatarActorFromActorInfo());
	ULSMonsterCombatComponent* CombatComponent = EnemyCharacter ? EnemyCharacter->GetMonsterCombatComponent() : nullptr;
	if (!EnemyCharacter || !CombatComponent)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 어떤 액션을 할지는 CombatComponent가 정하고(RequestAction), 몽타주는 그 액션 row의 Action_Ani에서 읽는다.
	const FLSMonsterActionRow* ActionRow = CombatComponent->GetActiveActionRow();
	ActiveActionMontage = ActionRow ? Cast<UAnimMontage>(ActionRow->Action_Ani.TryLoad()) : nullptr;
	if (!ActiveActionMontage)
	{
		UE_LOG(LogLS, Warning, TEXT("%s monster action has no montage resolved from Action_Ani."), *GetNameSafe(EnemyCharacter));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAnimInstance* AnimInstance = EnemyCharacter->GetMesh() ? EnemyCharacter->GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	EnemyCharacter->MulticastPlayAbilityMontage(ActiveActionMontage);
	if (!AnimInstance->Montage_IsPlaying(ActiveActionMontage))
	{
		UE_LOG(LogLS, Warning, TEXT("%s failed to play monster action montage %s."), *GetNameSafe(EnemyCharacter), *GetNameSafe(ActiveActionMontage));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FOnMontageEnded MontageEndedDelegate;
	MontageEndedDelegate.BindUObject(this, &ULSGA_MonsterAction::HandleActionMontageEnded);
	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, ActiveActionMontage);
}

void ULSGA_MonsterAction::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	bEndingAbility = true;

	if (ALSEnemyCharacter* EnemyCharacter = Cast<ALSEnemyCharacter>(GetAvatarActorFromActorInfo()))
	{
		// 캔슬/종료 시 텔레그래프가 남지 않도록 정리(노티파이 End가 누락된 경우 대비).
		if (ULSMonsterCombatComponent* CombatComponent = EnemyCharacter->GetMonsterCombatComponent())
		{
			CombatComponent->EndActionTelegraph();
		}

		if (UAnimInstance* AnimInstance = EnemyCharacter->GetMesh() ? EnemyCharacter->GetMesh()->GetAnimInstance() : nullptr)
		{
			if (ActiveActionMontage && AnimInstance->Montage_IsPlaying(ActiveActionMontage))
			{
				EnemyCharacter->MulticastStopAbilityMontage(ActiveActionMontage, 0.1f);
			}
		}
	}

	ActiveActionMontage = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void ULSGA_MonsterAction::HandleActionMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (bEndingAbility)
	{
		return;
	}

	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bInterrupted);
	}
}
