#include "Skills/LSBypassSkill.h"

#include "Combat/LSCharacterCombatComponent.h"
#include "Data/LSCharacterSkillRow.h"
#include "GAS/Abilities/LSGA_Bypass.h"
#include "GAS/LSGameplayTags.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "TimerManager.h"
#include "LostSignal.h"
#include "Skills/LSSkillDataAsset.h"

ULSBypassSkill::ULSBypassSkill()
{
	DefaultAbilityClass = ULSGA_Bypass::StaticClass();
	AttackCoefficient = 0.0f;
	BreakPower = ELSBreakPowerTier::NormalAttack;
}

bool ULSBypassSkill::ActivateSkill_Implementation(const FLSSkillActivationContext& Context)
{
	AActor* SourceActor = Context.SourceActor.Get();
	if (!SourceActor || !SourceActor->HasAuthority())
	{
		return false;
	}

	FLSCharacterSkillRow Row;
	const bool bHasRow = Context.SkillData && Context.SkillData->TryGetSkillRow(Row);
	const float Distance = bHasRow && Row.Range_X > 0.0f ? Row.Range_X : FallbackDistance;
	const float Duration = bHasRow && Row.Skill_Time > 0.0f ? Row.Skill_Time : FallbackDuration;
	if (Distance <= 0.0f || Duration <= 0.0f)
	{
		return false;
	}

	FVector AimDirection = Context.TargetLocation - SourceActor->GetActorLocation();
	AimDirection.Z = 0.0f;
	if (AimDirection.IsNearlyZero())
	{
		AimDirection = FRotator(0.0f, Context.AimYaw, 0.0f).Vector();
	}

	AimDirection = AimDirection.GetSafeNormal2D();
	if (AimDirection.IsNearlyZero())
	{
		AimDirection = SourceActor->GetActorForwardVector().GetSafeNormal2D();
	}

	if (AimDirection.IsNearlyZero())
	{
		return false;
	}

	ULSCharacterCombatComponent* CombatComponent = SourceActor->FindComponentByClass<ULSCharacterCombatComponent>();
	if (CombatComponent)
	{
		CombatComponent->SetCombatTagActive(LSGameplayTags::State_Invincible, true);

		FTimerDelegate TimerDelegate;
		TimerDelegate.BindUObject(this, &ULSBypassSkill::ClearBypassInvincibleTag, SourceActor);
		FTimerHandle InvincibleTimerHandle;
		SourceActor->GetWorldTimerManager().SetTimer(
			InvincibleTimerHandle,
			TimerDelegate,
			Duration,
			false);
	}

	const FVector StartLocation = SourceActor->GetActorLocation();
	const float SlideSpeed = Distance / Duration;
	ACharacter* SourceCharacter = Cast<ACharacter>(SourceActor);
	UCharacterMovementComponent* MovementComponent = SourceCharacter ? SourceCharacter->GetCharacterMovement() : nullptr;
	if (!MovementComponent)
	{
		ClearBypassInvincibleTag(SourceActor);
		return false;
	}

	TSharedPtr<FRootMotionSource_ConstantForce> RootMotion = MakeShared<FRootMotionSource_ConstantForce>();
	RootMotion->InstanceName = FName("Bypass");
	RootMotion->AccumulateMode = ERootMotionAccumulateMode::Override;
	RootMotion->Priority = 5;
	RootMotion->Force = AimDirection * SlideSpeed;
	RootMotion->Duration = Duration;
	RootMotion->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::SetVelocity;
	RootMotion->FinishVelocityParams.SetVelocity = FVector::ZeroVector;

	MovementComponent->ApplyRootMotionSource(RootMotion);

	if (bEnableDebugLog)
	{
		UE_LOG(
			LogLS,
			Log,
			TEXT("[Bypass] Source=%s Start=%s Direction=%s Distance=%.2f Duration=%.2f Speed=%.2f"),
			*GetNameSafe(SourceActor),
			*StartLocation.ToCompactString(),
			*AimDirection.ToCompactString(),
			Distance,
			Duration,
			SlideSpeed);
	}

	return true;
}

void ULSBypassSkill::ClearBypassInvincibleTag(AActor* SourceActor) const
{
	if (ULSCharacterCombatComponent* CombatComponent = SourceActor ? SourceActor->FindComponentByClass<ULSCharacterCombatComponent>() : nullptr)
	{
		CombatComponent->SetCombatTagActive(LSGameplayTags::State_Invincible, false);
	}
}
