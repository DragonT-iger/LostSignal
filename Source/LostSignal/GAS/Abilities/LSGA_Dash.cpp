#include "GAS/Abilities/LSGA_Dash.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS/LSGameplayTags.h"
#include "LostSignal.h"

ULSGA_Dash::ULSGA_Dash()
{
	// 이 어빌리티를 태그로 식별
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(LSGameplayTags::Ability_Dash);
	SetAssetTags(AssetTags);

	// 이미 대쉬 중이면 재발동 차단
	ActivationBlockedTags.AddTag(LSGameplayTags::State_Dodging);

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

	// ── 대쉬 이동 ────────────────────────────────────────────
	const float DashSpeed = DashDistance / FMath::Max(DashDuration, KINDA_SMALL_NUMBER);
	Character->LaunchCharacter(DashDir * DashSpeed, true, false);

	UE_LOG(LogLS, Verbose, TEXT("LSGA_Dash: 대쉬 시작 방향=%s 속도=%.0f"), *DashDir.ToString(), DashSpeed);

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

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DashTimerHandle);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
