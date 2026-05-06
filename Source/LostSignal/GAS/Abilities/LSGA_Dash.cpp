#include "GAS/Abilities/LSGA_Dash.h"

#include "AbilitySystemComponent.h"
#include "Combat/LSPlayerCombatComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "GAS/LSGameplayTags.h"
#include "GAS/Effects/LSGE_DashCooldown.h"
#include "GAS/Effects/LSGE_DashInvincible.h"
#include "NiagaraFunctionLibrary.h"
#include "LostSignal.h"
#include "../LSCharacterAttributeSet.h"

ULSGA_Dash::ULSGA_Dash()
{
	// 이 어빌리티를 태그로 식별
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(LSGameplayTags::Ability_Dash);
	SetAssetTags(AssetTags);

	// 무적(=대쉬 중)이면 재발동 차단
	ActivationBlockedTags.AddTag(LSGameplayTags::State_Invincible);

	// 쿨타임 태그 등록 — CommitAbility()가 GE_DashCooldown 적용 후 이 태그로 재발동 차단
	// CooldownGameplayEffectClass는 BP_GA_Dash Details에서 GE_DashCooldown 에셋 할당 필요
	CooldownTagContainer.AddTag(LSGameplayTags::Cooldown_Dash);

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	CooldownGameplayEffectClass = ULSGE_DashCooldown::StaticClass();
	InvincibilityEffectClass = ULSGE_DashInvincible::StaticClass();
}

const FGameplayTagContainer* ULSGA_Dash::GetCooldownTags() const
{
	return &CooldownTagContainer;
}

void ULSGA_Dash::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UE_LOG(LogLS, Warning, TEXT("대쉬 발동"));

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
	FVector DashDir = FVector::ZeroVector;
	if (const ULSPlayerCombatComponent* PlayerCombatComponent = Character->FindComponentByClass<ULSPlayerCombatComponent>())
	{
		PlayerCombatComponent->GetPendingDashDirection(DashDir);
	}
	if (DashDir.IsNearlyZero())
	{
		DashDir = Character->GetCharacterMovement()->GetLastInputVector();
		if (DashDir.IsNearlyZero())
		{
			DashDir = Character->GetActorForwardVector();
		}
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

	float AttributeDashSpeed    = ASC->GetNumericAttribute(ULSCharacterAttributeSet::GetDashSpeedAttribute());
	float AttributeDashDuration = ASC->GetNumericAttribute(ULSCharacterAttributeSet::GetDashDurationAttribute());
	if (AttributeDashDuration <= 0.f) { AttributeDashDuration = 0.3f; }

	// ── 대쉬 이동 (Root Motion Source) ──────────────────────
	TSharedPtr<FRootMotionSource_ConstantForce> RootMotion =
		MakeShared<FRootMotionSource_ConstantForce>();
	RootMotion->InstanceName    = FName("Dash");
	RootMotion->AccumulateMode  = ERootMotionAccumulateMode::Override;
	RootMotion->Priority        = 5;
	RootMotion->Force           = DashDir * AttributeDashSpeed;
	RootMotion->Duration        = AttributeDashDuration;
	RootMotion->FinishVelocityParams.Mode        = ERootMotionFinishVelocityMode::SetVelocity;
	RootMotion->FinishVelocityParams.SetVelocity = FVector::ZeroVector;

	RootMotionSourceID = Character->GetCharacterMovement()->ApplyRootMotionSource(RootMotion);

	UE_LOG(LogLS, Verbose, TEXT("LSGA_Dash: 대쉬 시작 방향=%s 속도=%.0f 시간=%.2f"),
		*DashDir.ToString(), AttributeDashSpeed, AttributeDashDuration);

	// ── 종료 타이머 ──────────────────────────────────────────
	GetWorld()->GetTimerManager().SetTimer(
		DashTimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			if (IsActive())
			{
				EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
			}
		}),
		AttributeDashDuration, false
	);
}

void ULSGA_Dash::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// 무적 GE 제거 — LocalPredicted이므로 서버(Authority)에서만 제거. 클라이언트의 예측 GE는 GAS가 자동 정리.
	if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
	{
		const AActor* AvatarActor = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
		if (AvatarActor && AvatarActor->HasAuthority() && InvincibilityHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(InvincibilityHandle);
		}
		InvincibilityHandle = FActiveGameplayEffectHandle();
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
