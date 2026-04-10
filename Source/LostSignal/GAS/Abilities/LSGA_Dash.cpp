#include "GAS/Abilities/LSGA_Dash.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "GAS/LSGameplayTags.h"
#include "NiagaraFunctionLibrary.h"
#include "LostSignal.h"

ULSGA_Dash::ULSGA_Dash()
{
	// 이 어빌리티를 태그로 식별
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(LSGameplayTags::Ability_Dash);
	SetAssetTags(AssetTags);

	// 무적(=대쉬 중)이면 재발동 차단
	ActivationBlockedTags.AddTag(LSGameplayTags::State_Invincible);

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void ULSGA_Dash::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();

	if (!Character || !ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// ── 대쉬 방향 ────────────────────────────────────────────
	FVector DashDir = Character->GetCharacterMovement()->GetLastInputVector();
	if (DashDir.IsNearlyZero())
	{
		DashDir = Character->GetActorForwardVector();
	}
	DashDir.Z = 0.f;
	DashDir = DashDir.GetSafeNormal();

	// ── 무적 GE 적용 ─────────────────────────────────────────
	if (InvincibilityEffectClass)
	{
		const FGameplayEffectSpecHandle SpecHandle =
			MakeOutgoingGameplayEffectSpec(InvincibilityEffectClass, GetAbilityLevel());
		InvincibilityHandle = ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("LSGA_Dash: InvincibilityEffectClass가 설정되지 않음 — 무적 없이 대쉬만 실행"));
	}

	// ── 대쉬 이동 (Root Motion Source) ──────────────────────
	// LaunchCharacter와 달리 DashDuration 동안 일정 속도를 유지하다 자동 종료.
	// Unity의 Vector3.MoveTowards를 코루틴으로 구현하는 것과 유사.
	TSharedPtr<FRootMotionSource_ConstantForce> RootMotion =
		MakeShared<FRootMotionSource_ConstantForce>();
	RootMotion->InstanceName    = FName("Dash");
	RootMotion->AccumulateMode  = ERootMotionAccumulateMode::Override; // 다른 이동 무시
	RootMotion->Priority        = 5;
	RootMotion->Force           = DashDir * DashSpeed;
	RootMotion->Duration        = DashDuration;
	// 대쉬 끝나면 속도 0으로 — MaintainLastRootMotionVelocity로 바꾸면 미끄러짐
	RootMotion->FinishVelocityParams.Mode        = ERootMotionFinishVelocityMode::SetVelocity;
	RootMotion->FinishVelocityParams.SetVelocity = FVector::ZeroVector;

	RootMotionSourceID = Character->GetCharacterMovement()->ApplyRootMotionSource(RootMotion);

	UE_LOG(LogLS, Verbose, TEXT("LSGA_Dash: 대쉬 시작 방향=%s 속도=%.0f 시간=%.2f"),
		*DashDir.ToString(), DashSpeed, DashDuration);

	// ── 종료 타이머 ──────────────────────────────────────────
	// Root Motion이 Duration 후 자동 만료되지만, GE 제거와 EndAbility는 직접 처리
	GetWorld()->GetTimerManager().SetTimer(
		DashTimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			if (IsActive())
			{
				EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
			}
		}),
		DashDuration, false
	);
}

void ULSGA_Dash::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// 무적 GE 제거
	if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
	{
		if (InvincibilityHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(InvincibilityHandle);
			InvincibilityHandle = FActiveGameplayEffectHandle();
		}
	}

	// Root Motion 강제 제거 (어빌리티 캔슬 시에도 이동이 즉시 멈춤)
	if (ACharacter* Character = Cast<ACharacter>(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr))
	{
		Character->GetCharacterMovement()->RemoveRootMotionSourceByID(RootMotionSourceID);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DashTimerHandle);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
