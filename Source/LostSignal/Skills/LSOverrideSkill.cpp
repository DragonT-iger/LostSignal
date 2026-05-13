#include "Skills/LSOverrideSkill.h"

#include "Combat/LSCharacterCombatComponent.h"
#include "Data/LSCharacterSkillRow.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffect.h"
#include "GAS/Abilities/LSGA_Override.h"
#include "Kismet/KismetSystemLibrary.h"
#include "LostSignal.h"

namespace
{
	ELSBreakPowerTier ToOverrideBreakPowerTier(int32 Value, ELSBreakPowerTier Fallback)
	{
		if (Value >= static_cast<int32>(ELSBreakPowerTier::HardCrowdControl))
		{
			return ELSBreakPowerTier::HardCrowdControl;
		}

		if (Value >= static_cast<int32>(ELSBreakPowerTier::SpecialAttack))
		{
			return ELSBreakPowerTier::SpecialAttack;
		}

		if (Value >= static_cast<int32>(ELSBreakPowerTier::NormalAttack))
		{
			return ELSBreakPowerTier::NormalAttack;
		}

		return Fallback;
	}
}

ULSOverrideSkill::ULSOverrideSkill()
{
	DefaultAbilityClass = ULSGA_Override::StaticClass();
	AttackCoefficient = 1.2f;
	BreakPower = ELSBreakPowerTier::NormalAttack;
}

bool ULSOverrideSkill::ActivateSkill_Implementation(const FLSSkillActivationContext& Context)
{
	AActor* SourceActor = Context.SourceActor.Get();
	if (!SourceActor || !SourceActor->HasAuthority())
	{
		return false;
	}

	ULSCharacterCombatComponent* SourceCombatComponent = SourceActor->FindComponentByClass<ULSCharacterCombatComponent>();
	if (!SourceCombatComponent || !DamageEffectClass)
	{
		return false;
	}

	FLSCharacterSkillRow Row;
	const bool bHasRow = TryGetSkillRow(Row);
	const float Radius = bHasRow && Row.Range_X > 0.0f ? Row.Range_X : FallbackRadius;
	const float ResolvedAttackCoefficient = bHasRow && Row.Skill_Multiplier > 0.0f ? Row.Skill_Multiplier : FallbackAttackCoefficient;
	const ELSBreakPowerTier ResolvedBreakPower = bHasRow ? ToOverrideBreakPowerTier(Row.Skill_Impact, BreakPower) : BreakPower;
	if (Radius <= 0.0f)
	{
		return false;
	}

	TArray<AActor*> OverlappedActors;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(SourceActor);
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	const FVector SourceLocation = SourceActor->GetActorLocation();
	UKismetSystemLibrary::SphereOverlapActors(
		SourceActor->GetWorld(),
		SourceLocation,
		Radius,
		ObjectTypes,
		nullptr,
		ActorsToIgnore,
		OverlappedActors);

	int32 ValidHitCount = 0;
	int32 KnockbackCount = 0;
	for (AActor* TargetActor : OverlappedActors)
	{
		if (!TargetActor)
		{
			continue;
		}

		if (!SourceCombatComponent->ApplyDamageEffectToTarget(
			TargetActor,
			DamageEffectClass,
			1.0f,
			FixedDamage,
			ResolvedAttackCoefficient,
			bCanCrit,
			ResolvedBreakPower))
		{
			continue;
		}

		++ValidHitCount;

		const ULSCharacterCombatComponent* TargetCombatComponent = TargetActor->FindComponentByClass<ULSCharacterCombatComponent>();
		if (TargetCombatComponent && !TargetCombatComponent->CanApplyCrowdControl(ResolvedBreakPower))
		{
			continue;
		}

		ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor);
		if (!TargetCharacter)
		{
			continue;
		}

		FVector KnockbackDirection = TargetActor->GetActorLocation() - SourceLocation;
		KnockbackDirection.Z = 0.0f;
		if (KnockbackDirection.IsNearlyZero())
		{
			KnockbackDirection = SourceActor->GetActorForwardVector();
		}

		KnockbackDirection = KnockbackDirection.GetSafeNormal2D();
		if (AController* TargetController = TargetCharacter->GetController())
		{
			TargetController->StopMovement();
		}

		if (UCharacterMovementComponent* MovementComponent = TargetCharacter->GetCharacterMovement())
		{
			MovementComponent->StopMovementImmediately();
		}

		TargetCharacter->LaunchCharacter(
			(KnockbackDirection * KnockbackSpeed) + FVector(0.0f, 0.0f, KnockbackUpSpeed),
			true,
			true);
		++KnockbackCount;
	}

	if (bEnableDebugLog)
	{
		UE_LOG(
			LogLS,
			Log,
			TEXT("[Override] Source=%s Radius=%.2f RawTargets=%d ValidHits=%d Knockbacks=%d Coef=%.2f BreakPower=%d"),
			*GetNameSafe(SourceActor),
			Radius,
			OverlappedActors.Num(),
			ValidHitCount,
			KnockbackCount,
			ResolvedAttackCoefficient,
			static_cast<int32>(ResolvedBreakPower));
	}

	return true;
}
