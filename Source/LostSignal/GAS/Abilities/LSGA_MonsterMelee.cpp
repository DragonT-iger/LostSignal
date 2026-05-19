#include "GAS/Abilities/LSGA_MonsterMelee.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Characters/LSEnemyCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "GAS/LSGameplayTags.h"
#include "LostSignal.h"

ULSGA_MonsterMelee::ULSGA_MonsterMelee()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(LSGameplayTags::Ability_MonsterMelee);
	SetAssetTags(AssetTags);

	ActivationOwnedTags.AddTag(LSGameplayTags::Combat_Attacking);
	ActivationBlockedTags.AddTag(LSGameplayTags::Combat_Attacking);
	ActivationBlockedTags.AddTag(LSGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(LSGameplayTags::State_Stunned);

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void ULSGA_MonsterMelee::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	bEndingAbility = false;

	ALSEnemyCharacter* EnemyCharacter = Cast<ALSEnemyCharacter>(GetAvatarActorFromActorInfo());
	if (!EnemyCharacter)
	{
		UE_LOG(LogLS, Warning, TEXT("MonsterMelee ActivateAbility failed: avatar is not ALSEnemyCharacter."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// The ability decides "what action" to run, but the monster character owns "which montage" to play.
	const FGameplayTag MeleeAbilityTag = LSGameplayTags::Ability_MonsterMelee;
	ActiveAttackMontage = EnemyCharacter->GetAbilityMontage(MeleeAbilityTag);
	if (!ActiveAttackMontage)
	{
		UE_LOG(LogLS, Warning, TEXT("%s has no montage mapped for %s."), *GetNameSafe(EnemyCharacter), *MeleeAbilityTag.ToString());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		UE_LOG(LogLS, Warning, TEXT("%s MonsterMelee CommitAbility failed."), *GetNameSafe(EnemyCharacter));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAnimInstance* AnimInstance = EnemyCharacter->GetMesh() ? EnemyCharacter->GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		UE_LOG(LogLS, Warning, TEXT("%s MonsterMelee has no AnimInstance."), *GetNameSafe(EnemyCharacter));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	EnemyCharacter->MulticastPlayAbilityMontage(ActiveAttackMontage);
	if (!AnimInstance->Montage_IsPlaying(ActiveAttackMontage))
	{
		UE_LOG(LogLS, Warning, TEXT("%s MonsterMelee failed to play montage %s."), *GetNameSafe(EnemyCharacter), *GetNameSafe(ActiveAttackMontage));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FOnMontageEnded MontageEndedDelegate;
	MontageEndedDelegate.BindUObject(this, &ULSGA_MonsterMelee::HandleAttackMontageEnded);
	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, ActiveAttackMontage);

	UE_LOG(LogLS, Log, TEXT("%s activated monster melee ability."), *GetNameSafe(EnemyCharacter));
}

void ULSGA_MonsterMelee::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	bEndingAbility = true;

	UE_LOG(
		LogLS,
		Log,
		TEXT("%s EndAbility called. Cancelled=%d ReplicateEnd=%d Montage=%s"),
		*GetNameSafe(GetAvatarActorFromActorInfo()),
		bWasCancelled,
		bReplicateEndAbility,
		*GetNameSafe(ActiveAttackMontage)
	);

	if (ALSEnemyCharacter* EnemyCharacter = Cast<ALSEnemyCharacter>(GetAvatarActorFromActorInfo()))
	{
		if (UAnimInstance* AnimInstance = EnemyCharacter->GetMesh() ? EnemyCharacter->GetMesh()->GetAnimInstance() : nullptr)
		{
			if (ActiveAttackMontage && AnimInstance->Montage_IsPlaying(ActiveAttackMontage))
			{
				EnemyCharacter->MulticastStopAbilityMontage(ActiveAttackMontage, 0.1f);
			}
		}
	}

	ActiveAttackMontage = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void ULSGA_MonsterMelee::HandleAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (bEndingAbility)
	{
		return;
	}

	UE_LOG(
		LogLS,
		Log,
		TEXT("%s melee montage ended. Interrupted=%d Montage=%s"),
		*GetNameSafe(GetAvatarActorFromActorInfo()),
		bInterrupted,
		*GetNameSafe(Montage)
	);

	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bInterrupted);
	}
}
