#include "GAS/Abilities/Character1/LSGA_PlayerSkillBase.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Characters/LSCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "GAS/LSGameplayTags.h"
#include "LostSignal.h"
#include "Skills/LSPlayerSkillComponent.h"
#include "Skills/LSSkillDataAsset.h"

ULSGA_PlayerSkillBase::ULSGA_PlayerSkillBase()
{
	// 공통 차단/캔슬 태그 계약(SkillSystemStructure.md "기본 공격 캔슬과 스킬 차단 태그" 단일 출처).
	ActivationBlockedTags.AddTag(LSGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(LSGameplayTags::State_Stunned);
	ActivationBlockedTags.AddTag(LSGameplayTags::Combat_SkillCasting); // 스킬끼리만 차단 (기본공격은 통과)
	ActivationOwnedTags.AddTag(LSGameplayTags::Combat_Attacking);      // 공통 "진행 중" 의미 유지
	ActivationOwnedTags.AddTag(LSGameplayTags::Combat_SkillCasting);   // 시전 중 표식
	CancelAbilitiesWithTag.AddTag(LSGameplayTags::Ability_PlayerBasicAttack); // 기본공격 모션 캔슬 후 발동

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	SkillEffectEventTag = LSGameplayTags::Event_Skill_Hit;
}

AActor* ULSGA_PlayerSkillBase::GetSkillSourceActor() const
{
	return GetAvatarActorFromActorInfo();
}

UAnimMontage* ULSGA_PlayerSkillBase::GetSkillMontage() const
{
	return SkillContext.SkillData ? SkillContext.SkillData->SkillMontage : nullptr;
}

float ULSGA_PlayerSkillBase::ComputeMontagePlayRateForDuration(const UAnimMontage* Montage, FName SectionName, float TargetDuration) const
{
	if (!Montage || TargetDuration <= 0.0f)
	{
		return 1.0f;
	}

	float SourceLength = 0.0f;
	if (SectionName.IsNone())
	{
		SourceLength = Montage->GetPlayLength();
	}
	else
	{
		const int32 SectionIndex = Montage->GetSectionIndex(SectionName);
		SourceLength = SectionIndex != INDEX_NONE ? Montage->GetSectionLength(SectionIndex) : 0.0f;
	}

	if (SourceLength <= 0.0f)
	{
		return 1.0f;
	}

	const float RawPlayRate = SourceLength / TargetDuration;
	const float ClampedPlayRate = FMath::Clamp(RawPlayRate, 0.25f, 3.0f);
	if (!FMath::IsNearlyEqual(RawPlayRate, ClampedPlayRate))
	{
		UE_LOG(LogLS, Warning,
			TEXT("[%s] 스킬 몽타주 playRate 클램프: raw=%.2f clamped=%.2f (몽타주 길이=%.2f, 목표=%.2f)"),
			*GetName(), RawPlayRate, ClampedPlayRate, SourceLength, TargetDuration);
	}
	return ClampedPlayRate;
}

void ULSGA_PlayerSkillBase::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	bSkillEffectExecuted = false;
	bEndingAbility = false;
	ActiveSkillMontage = nullptr;
	SkillContext = FLSSkillActivationContext();

	AActor* SourceActor = GetAvatarActorFromActorInfo();
	ULSPlayerSkillComponent* SkillComponent = SourceActor ? SourceActor->FindComponentByClass<ULSPlayerSkillComponent>() : nullptr;
	if (!SourceActor || !SourceActor->HasAuthority() || !SkillComponent)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!SkillComponent->ConsumePendingAbilityContext(GetClass(), SkillContext) || !SkillContext.SkillData)
	{
		UE_LOG(LogLS, Warning, TEXT("%s skill ability missing pending skill context."), *GetNameSafe(SourceActor));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 서브클래스 검증/캐싱 실패 시 커밋·쿨타임 없이 취소(기존 즉발 경로의 조기 반환과 동일 의미).
	if (!PrepareSkillExecution())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	SkillComponent->ApplySkillCooldown(SkillContext.SkillData);

	// "발동 즉시" 세팅(이동 루트모션·버프 등). 몽타주 playRate 계산에 쓸 값도 여기서 캐싱된다.
	OnSkillStarted();

	const bool bMontageDrivesEnd = ShouldMontageDriveEnd();

	UAnimMontage* Montage = SkillContext.SkillData->SkillMontage;
	ALSCharacterBase* Character = Cast<ALSCharacterBase>(SourceActor);
	UAnimInstance* AnimInstance = Character && Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;

	// 몽타주 미할당이거나 재생 환경이 없는 경우.
	if (!Montage || !Character || !AnimInstance)
	{
		if (bMontageDrivesEnd)
		{
			// 즉발형: 효과를 즉시 실행하고 종료한다.
			TriggerSkillEffectOnce();
			EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		}
		// bMontageDrivesEnd==false면 서브클래스 타이머가 종료를 책임진다(여기선 아무것도 안 함).
		return;
	}

	ActiveSkillMontage = Montage;

	// 노티파이 이벤트 대기 태스크를 먼저 활성화한 뒤 몽타주를 재생한다(노티파이 없는 스킬엔 무해).
	UAbilityTask_WaitGameplayEvent* WaitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, SkillEffectEventTag, nullptr, /*OnlyTriggerOnce=*/false, /*OnlyMatchExact=*/true);
	if (WaitTask)
	{
		WaitTask->EventReceived.AddDynamic(this, &ULSGA_PlayerSkillBase::OnSkillEffectEventReceived);
		WaitTask->ReadyForActivation();
	}

	// 몽타주는 비주얼. 이동 스킬은 playRate로 길이를 이동 Duration에 맞춘다.
	Character->MulticastPlayLSMontage(Montage, NAME_None, GetSkillMontagePlayRate());

	if (!bMontageDrivesEnd)
	{
		// 종료는 서브클래스 타이머가 책임진다. 몽타주 종료 델리게이트를 바인딩하지 않는다.
		return;
	}

	if (!AnimInstance->Montage_IsPlaying(Montage))
	{
		// 재생 실패 시 효과가 영영 안 나가지 않도록 즉발 fallback 후 종료.
		TriggerSkillEffectOnce();
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	FOnMontageEnded MontageEndedDelegate;
	MontageEndedDelegate.BindUObject(this, &ULSGA_PlayerSkillBase::HandleSkillMontageEnded);
	AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, Montage);
}

void ULSGA_PlayerSkillBase::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	bEndingAbility = true;

	// 윈드업 중 캔슬(스턴/사망 등)이면 몽타주를 멈춘다. 효과는 가드로 인해 발동하지 않는다.
	if (bWasCancelled && ActiveSkillMontage)
	{
		if (ALSCharacterBase* Character = Cast<ALSCharacterBase>(GetAvatarActorFromActorInfo()))
		{
			Character->MulticastStopLSMontage(ActiveSkillMontage, 0.1f);
		}
	}

	ActiveSkillMontage = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void ULSGA_PlayerSkillBase::TriggerSkillEffectOnce()
{
	if (bSkillEffectExecuted)
	{
		return;
	}

	bSkillEffectExecuted = true;
	ExecuteSkillEffect();
}

void ULSGA_PlayerSkillBase::OnSkillEffectEventReceived(FGameplayEventData Payload)
{
	if (!IsActive())
	{
		return;
	}

	TriggerSkillEffectOnce();
}

void ULSGA_PlayerSkillBase::HandleSkillMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (bEndingAbility || !IsActive())
	{
		return;
	}

	// 노티파이가 누락된 몽타주라도 종료 전에 효과를 보장한다(정상 종료에 한함).
	if (!bInterrupted)
	{
		TriggerSkillEffectOnce();
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bInterrupted);
}
