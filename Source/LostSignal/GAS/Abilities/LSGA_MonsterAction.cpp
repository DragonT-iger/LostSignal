#include "GAS/Abilities/LSGA_MonsterAction.h"

#include "AI/LSMonsterCombatComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Characters/Enemys/LSEnemyCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Data/LSMonsterActionRow.h"
#include "GAS/LSGameplayTags.h"
#include "LostSignal.h"

namespace
{
	const FName MonsterActionChargeStartSectionName(TEXT("ChargeStart"));
	const FName MonsterActionChargeLoopSectionName(TEXT("ChargeLoop"));
	const FName MonsterActionChargeHitSectionName(TEXT("ChargeHit"));
	const FName MonsterActionChargeMissSectionName(TEXT("ChargeMiss"));
}

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

	BindActionChargeDelegates(CombatComponent);

	// Chase에서 남은 MoveTo와 속도를 끊어 공격 선딜 중 플레이어를 계속 따라가지 않게 한다.
	// MovementComponent 자체는 유지해야 이후 도약/돌진 RootMotionSource가 이동할 수 있다.
	if (AController* Controller = EnemyCharacter->GetController())
	{
		Controller->StopMovement();
	}

	// 공격 중에는 플레이어를 향한 body 회전을 멈춰 공격 방향을 시작 시점으로 고정한다(종료 시 복원).
	if (UCharacterMovementComponent* MovementComponent = EnemyCharacter->GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
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
	ULSMonsterCombatComponent* CombatComponent = ActiveCombatComponent.Get();
	UnbindActionChargeDelegates();

	if (ALSEnemyCharacter* EnemyCharacter = Cast<ALSEnemyCharacter>(GetAvatarActorFromActorInfo()))
	{
		// 캔슬/종료 시 텔레그래프·도약 이동이 남지 않도록 정리(노티파이 End가 누락된 경우 대비).
		if (!CombatComponent)
		{
			CombatComponent = EnemyCharacter->GetMonsterCombatComponent();
		}
		if (CombatComponent)
		{
			CombatComponent->EndActionTelegraph();
			CombatComponent->CancelActionCharge();
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
	ActiveCombatComponent.Reset();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void ULSGA_MonsterAction::HandleActionChargeStarted()
{
	ALSEnemyCharacter* EnemyCharacter = Cast<ALSEnemyCharacter>(GetAvatarActorFromActorInfo());
	if (!EnemyCharacter || !IsActive() || !HasValidActionChargeSections())
	{
		UE_LOG(LogLS, Warning, TEXT("%s 충돌형 돌진 몽타주에 ChargeStart/ChargeLoop 섹션이 모두 필요합니다."), *GetNameSafe(EnemyCharacter));
		if (ULSMonsterCombatComponent* CombatComponent = ActiveCombatComponent.Get())
		{
			CombatComponent->CancelActionCharge();
		}
		if (IsActive())
		{
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		}
		return;
	}

	// [돌진 진단] 루프 연결 직전 현재 섹션 상태 확인.
	UAnimInstance* DiagAnim = EnemyCharacter->GetMesh() ? EnemyCharacter->GetMesh()->GetAnimInstance() : nullptr;
	UE_LOG(LogLS, Log, TEXT("[돌진] ChargeStarted 처리 %s: 현재섹션=%s, Start/Loop 연결"),
		*GetNameSafe(EnemyCharacter),
		DiagAnim ? *DiagAnim->Montage_GetCurrentSection(ActiveActionMontage).ToString() : TEXT("<no anim>"));

	EnemyCharacter->MulticastSetLSMontageNextSection(ActiveActionMontage, MonsterActionChargeStartSectionName, MonsterActionChargeLoopSectionName);
	EnemyCharacter->MulticastSetLSMontageNextSection(ActiveActionMontage, MonsterActionChargeLoopSectionName, MonsterActionChargeLoopSectionName);
}

void ULSGA_MonsterAction::HandleActionChargeFinished(bool bHit)
{
	ALSEnemyCharacter* EnemyCharacter = Cast<ALSEnemyCharacter>(GetAvatarActorFromActorInfo());
	if (!EnemyCharacter || !IsActive() || !ActiveActionMontage)
	{
		return;
	}

	const FName ResultSection = bHit ? MonsterActionChargeHitSectionName : MonsterActionChargeMissSectionName;
	if (ActiveActionMontage->IsValidSectionName(ResultSection))
	{
		EnemyCharacter->MulticastSetLSMontageNextSection(ActiveActionMontage, ResultSection, NAME_None);
		EnemyCharacter->MulticastJumpLSMontageSection(ActiveActionMontage, ResultSection);
		return;
	}

	// 결과 섹션이 없으면 현재 ChargeLoop 재생은 유지하고 다음 연결만 끊어, 이번 루프 끝에서 몽타주를 자연 종료한다.
	EnemyCharacter->MulticastSetLSMontageNextSection(ActiveActionMontage, MonsterActionChargeLoopSectionName, NAME_None);
}

void ULSGA_MonsterAction::BindActionChargeDelegates(ULSMonsterCombatComponent* CombatComponent)
{
	UnbindActionChargeDelegates();
	if (!CombatComponent)
	{
		return;
	}

	ActiveCombatComponent = CombatComponent;
	ActionChargeStartedHandle = CombatComponent->OnActionChargeStarted().AddUObject(this, &ULSGA_MonsterAction::HandleActionChargeStarted);
	ActionChargeFinishedHandle = CombatComponent->OnActionChargeFinished().AddUObject(this, &ULSGA_MonsterAction::HandleActionChargeFinished);
}

void ULSGA_MonsterAction::UnbindActionChargeDelegates()
{
	if (ULSMonsterCombatComponent* CombatComponent = ActiveCombatComponent.Get())
	{
		CombatComponent->OnActionChargeStarted().Remove(ActionChargeStartedHandle);
		CombatComponent->OnActionChargeFinished().Remove(ActionChargeFinishedHandle);
	}

	ActionChargeStartedHandle.Reset();
	ActionChargeFinishedHandle.Reset();
}

bool ULSGA_MonsterAction::HasValidActionChargeSections() const
{
	return ActiveActionMontage &&
		ActiveActionMontage->IsValidSectionName(MonsterActionChargeStartSectionName) &&
		ActiveActionMontage->IsValidSectionName(MonsterActionChargeLoopSectionName);
}

void ULSGA_MonsterAction::HandleActionMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// [돌진 진단] 몽타주 종료로 어빌리티가 끝나는지 확인.
	UE_LOG(LogLS, Log, TEXT("[돌진] MontageEnded %s: interrupted=%d, bEndingAbility=%d"),
		*GetNameSafe(GetAvatarActorFromActorInfo()), bInterrupted ? 1 : 0, bEndingAbility ? 1 : 0);

	if (bEndingAbility)
	{
		return;
	}

	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bInterrupted);
	}
}
