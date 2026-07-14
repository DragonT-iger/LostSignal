#include "GAS/Abilities/LSGA_MonsterAction.h"

#include "AI/LSMonsterCombatComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Characters/Enemys/LSEnemyCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Data/LSMonsterActionRow.h"
#include "GAS/LSGameplayTags.h"
#include "LostSignal.h"

ULSGA_MonsterAction::ULSGA_MonsterAction()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(LSGameplayTags::Ability_MonsterAction);
	AssetTags.AddTag(LSGameplayTags::Combat_Attacking); // 스턴·사망 등 외부 CancelAbilities 매칭용 분류 태그(AssetTags 기준 매칭)
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
		UE_LOG(LogLS, Warning, TEXT("MonsterAction ActivateAbility aborted: EnemyCharacter=%s CombatComponent=%s"),
			*GetNameSafe(EnemyCharacter), *GetNameSafe(CombatComponent));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 어떤 액션을 할지는 CombatComponent가 정하고(RequestAction), 몽타주는 그 액션 row의 Action_Ani에서 읽는다.
	const FLSMonsterActionRow* ActionRow = CombatComponent->GetActiveActionRow();
	ActiveActionMontage = ActionRow ? Cast<UAnimMontage>(ActionRow->Action_Ani.TryLoad()) : nullptr;

	// 진단: 활성 row가 잡혔는지, Action_Ani 경로에서 몽타주가 실제로 로드됐는지 확인.
	UE_LOG(LogLS, Log, TEXT("MonsterAction %s: row=%s, Action_Ani=%s -> montage=%s"),
		*GetNameSafe(EnemyCharacter),
		ActionRow ? *ActionRow->Action_Name.ToString() : TEXT("<no active row>"),
		ActionRow ? *ActionRow->Action_Ani.ToString() : TEXT("<no active row>"),
		*GetNameSafe(ActiveActionMontage));

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

	// 공격 중에는 플레이어를 향한 body 회전을 멈춰 공격 방향을 시작 시점으로 고정한다(종료 시 복원).
	if (UCharacterMovementComponent* MovementComponent = EnemyCharacter->GetCharacterMovement())
	{
		bSavedUseControllerDesiredRotation = MovementComponent->bUseControllerDesiredRotation;
		MovementComponent->bUseControllerDesiredRotation = false;
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
		// 캔슬/종료 시 텔레그래프·도약 이동이 남지 않도록 정리(노티파이 End가 누락된 경우 대비).
		if (ULSMonsterCombatComponent* CombatComponent = EnemyCharacter->GetMonsterCombatComponent())
		{
			CombatComponent->EndActionTelegraph();
			CombatComponent->EndActionDash();
		}

		if (UAnimInstance* AnimInstance = EnemyCharacter->GetMesh() ? EnemyCharacter->GetMesh()->GetAnimInstance() : nullptr)
		{
			if (ActiveActionMontage && AnimInstance->Montage_IsPlaying(ActiveActionMontage))
			{
				EnemyCharacter->MulticastStopAbilityMontage(ActiveActionMontage, 0.5f);
			}
		}

		// 공격 종료 — body 회전 복원(공격 사이엔 다시 플레이어를 향한다).
		if (UCharacterMovementComponent* MovementComponent = EnemyCharacter->GetCharacterMovement())
		{
			MovementComponent->bUseControllerDesiredRotation = bSavedUseControllerDesiredRotation;
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
