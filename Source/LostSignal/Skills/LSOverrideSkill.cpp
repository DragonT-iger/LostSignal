#include "Skills/LSOverrideSkill.h"

#include "Combat/LSCharacterCombatComponent.h"
#include "Data/LSCharacterSkillRow.h"
#include "Engine/EngineTypes.h"
#include "GameplayEffect.h"
#include "GAS/Abilities/Character1/LSGA_Override.h"
#include "Kismet/KismetSystemLibrary.h"
#include "LostSignal.h"
#include "Skills/LSSkillDataAsset.h"

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

	const TSubclassOf<UGameplayEffect> ResolvedDamageEffectClass = Context.SkillData && Context.SkillData->DamageEffectClass
		? Context.SkillData->DamageEffectClass
		: DamageEffectClass;
	ULSCharacterCombatComponent* SourceCombatComponent = SourceActor->FindComponentByClass<ULSCharacterCombatComponent>();
	if (!SourceCombatComponent || !ResolvedDamageEffectClass)
	{
		return false;
	}

	FLSCharacterSkillRow Row;
	const bool bHasRow = Context.SkillData && Context.SkillData->TryGetSkillRow(Row);
	const float Radius = bHasRow && Row.Range_X > 0.0f ? Row.Range_X : FallbackRadius;
	const float DataAssetAttackCoefficient = Context.SkillData && Context.SkillData->AttackCoefficient > 0.0f ? Context.SkillData->AttackCoefficient : FallbackAttackCoefficient;
	const float ResolvedAttackCoefficient = bHasRow && Row.Skill_Multiplier > 0.0f ? Row.Skill_Multiplier : DataAssetAttackCoefficient;
	const ELSBreakPowerTier DataAssetBreakPower = Context.SkillData ? Context.SkillData->BreakPower : BreakPower;
	const ELSBreakPowerTier ResolvedBreakPower = bHasRow ? ToOverrideBreakPowerTier(Row.Skill_Impact, DataAssetBreakPower) : DataAssetBreakPower;
	const float ResolvedKnockbackDuration = bHasRow && Row.Skill_Time > 0.0f ? Row.Skill_Time : FallbackKnockbackDuration;
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
			ResolvedDamageEffectClass,
			1.0f,
			Context.SkillData ? Context.SkillData->FixedDamage : FixedDamage,
			ResolvedAttackCoefficient,
			Context.SkillData ? Context.SkillData->bCanCrit : bCanCrit,
			ResolvedBreakPower))
		{
			continue;
		}

		++ValidHitCount;

		ULSCharacterCombatComponent* TargetCombatComponent = TargetActor->FindComponentByClass<ULSCharacterCombatComponent>();
		if (TargetCombatComponent && !TargetCombatComponent->CanApplyCrowdControl(ResolvedBreakPower))
		{
			continue;
		}

		if (!TargetCombatComponent)
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
		if (TargetCombatComponent->ApplyKnockback(KnockbackDirection, KnockbackSpeed, ResolvedKnockbackDuration, KnockbackUpSpeed))
		{
			++KnockbackCount;
		}
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
