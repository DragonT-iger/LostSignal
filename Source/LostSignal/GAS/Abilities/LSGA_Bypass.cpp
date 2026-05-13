#include "GAS/Abilities/LSGA_Bypass.h"

#include "Combat/LSCharacterCombatComponent.h"
#include "Data/LSCharacterSkillRow.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "GAS/LSGameplayTags.h"
#include "LostSignal.h"
#include "Skills/LSPlayerSkillComponent.h"
#include "Skills/LSSkillDataAsset.h"
#include "TimerManager.h"

ULSGA_Bypass::ULSGA_Bypass()
{
	ActivationBlockedTags.AddTag(LSGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(LSGameplayTags::Combat_Attacking);
	ActivationOwnedTags.AddTag(LSGameplayTags::Combat_Attacking);

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void ULSGA_Bypass::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	ACharacter* SourceCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	AActor* SourceActor = SourceCharacter;
	ULSPlayerSkillComponent* SkillComponent = SourceActor ? SourceActor->FindComponentByClass<ULSPlayerSkillComponent>() : nullptr;
	if (!SourceActor || !SourceActor->HasAuthority() || !SourceCharacter || !SkillComponent)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FLSSkillActivationContext SkillContext;
	if (!SkillComponent->ConsumePendingAbilityContext(GetClass(), SkillContext) || !SkillContext.SkillData)
	{
		UE_LOG(LogLS, Warning, TEXT("%s Bypass ability missing pending skill context."), *GetNameSafe(SourceActor));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	float Distance = 0.0f;
	float Duration = 0.0f;
	if (!ResolveMovementParams(SkillContext.SkillData, Distance, Duration))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FVector AimDirection = SkillContext.TargetLocation - SourceActor->GetActorLocation();
	AimDirection.Z = 0.0f;
	if (AimDirection.IsNearlyZero())
	{
		AimDirection = FRotator(0.0f, SkillContext.AimYaw, 0.0f).Vector();
	}

	AimDirection = AimDirection.GetSafeNormal2D();
	if (AimDirection.IsNearlyZero())
	{
		AimDirection = SourceActor->GetActorForwardVector().GetSafeNormal2D();
	}

	UCharacterMovementComponent* MovementComponent = SourceCharacter->GetCharacterMovement();
	if (AimDirection.IsNearlyZero() || !MovementComponent)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	SetInvincibleTagActive(true);

	const float SlideSpeed = Distance / Duration;
	TSharedPtr<FRootMotionSource_ConstantForce> RootMotion = MakeShared<FRootMotionSource_ConstantForce>();
	RootMotion->InstanceName = FName("Bypass");
	RootMotion->AccumulateMode = ERootMotionAccumulateMode::Override;
	RootMotion->Priority = 5;
	RootMotion->Force = AimDirection * SlideSpeed;
	RootMotion->Duration = Duration;
	RootMotion->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::SetVelocity;
	RootMotion->FinishVelocityParams.SetVelocity = FVector::ZeroVector;

	RootMotionSourceID = MovementComponent->ApplyRootMotionSource(RootMotion);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(BypassTimerHandle, this, &ULSGA_Bypass::FinishBypass, Duration, false);
	}

	if (bEnableDebugLog)
	{
		UE_LOG(
			LogLS,
			Log,
			TEXT("[GA_Bypass] Source=%s Direction=%s Distance=%.2f Duration=%.2f Speed=%.2f"),
			*GetNameSafe(SourceActor),
			*AimDirection.ToCompactString(),
			Distance,
			Duration,
			SlideSpeed);
	}
}

bool ULSGA_Bypass::ResolveMovementParams(const ULSSkillDataAsset* SkillData, float& OutDistance, float& OutDuration) const
{
	FLSCharacterSkillRow Row;
	const bool bHasRow = SkillData && SkillData->TryGetSkillRow(Row);
	OutDistance = bHasRow && Row.Range_X > 0.0f ? Row.Range_X : FallbackDistance;
	OutDuration = bHasRow && Row.Skill_Time > 0.0f ? Row.Skill_Time : FallbackDuration;
	return OutDistance > 0.0f && OutDuration > 0.0f;
}

void ULSGA_Bypass::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BypassTimerHandle);
	}

	if (ACharacter* SourceCharacter = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
	{
		if (UCharacterMovementComponent* MovementComponent = SourceCharacter->GetCharacterMovement())
		{
			MovementComponent->RemoveRootMotionSourceByID(RootMotionSourceID);
		}
	}

	SetInvincibleTagActive(false);
	RootMotionSourceID = 0;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void ULSGA_Bypass::FinishBypass()
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void ULSGA_Bypass::SetInvincibleTagActive(bool bActive)
{
	if (bInvincibleTagActive == bActive)
	{
		return;
	}

	if (AActor* SourceActor = GetAvatarActorFromActorInfo())
	{
		if (ULSCharacterCombatComponent* CombatComponent = SourceActor->FindComponentByClass<ULSCharacterCombatComponent>())
		{
			CombatComponent->SetCombatTagActive(LSGameplayTags::State_Invincible, bActive);
			bInvincibleTagActive = bActive;
		}
	}
}
