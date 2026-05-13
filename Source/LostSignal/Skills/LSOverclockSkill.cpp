#include "Skills/LSOverclockSkill.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Combat/LSCharacterCombatComponent.h"
#include "Data/LSCharacterSkillRow.h"
#include "Engine/EngineTypes.h"
#include "GAS/LSGameplayTags.h"
#include "Kismet/KismetSystemLibrary.h"
#include "LostSignal.h"

namespace
{
	ELSBreakPowerTier ToBreakPowerTier(int32 Value, ELSBreakPowerTier Fallback)
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

ULSOverclockSkill::ULSOverclockSkill()
{
	AttackCoefficient = 2.5f;
	BreakPower = ELSBreakPowerTier::NormalAttack;
}

bool ULSOverclockSkill::ActivateSkill_Implementation(const FLSSkillActivationContext& Context)
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
	const float Range = bHasRow && Row.Range_X > 0.0f ? Row.Range_X : FallbackRange;
	const float ConeDegrees = bHasRow && Row.Range_Y > 0.0f ? Row.Range_Y : FallbackConeDegrees;
	const float BaseAttackCoefficient = bHasRow && Row.Skill_Multiplier > 0.0f ? Row.Skill_Multiplier : AttackCoefficient;
	const float AdditionalCoefficientPerStack = bHasRow && Row.Skill_Count_Multiplier > 0.0f ? Row.Skill_Count_Multiplier : FallbackAdditionalAttackCoefficientPerStack;
	const ELSBreakPowerTier ResolvedBreakPower = bHasRow ? ToBreakPowerTier(Row.Skill_Impact, BreakPower) : BreakPower;

	const FVector SourceLocation = SourceActor->GetActorLocation();
	FVector AimDirection = Context.TargetLocation - SourceLocation;
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

	TArray<AActor*> OverlappedActors;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(SourceActor);
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	UKismetSystemLibrary::SphereOverlapActors(
		SourceActor->GetWorld(),
		SourceLocation,
		Range,
		ObjectTypes,
		nullptr,
		ActorsToIgnore,
		OverlappedActors);

	const float CosHalfAngle = FMath::Cos(FMath::DegreesToRadians(ConeDegrees * 0.5f));
	TArray<AActor*> ConeTargets;
	for (AActor* TargetActor : OverlappedActors)
	{
		if (!TargetActor || ConeTargets.Contains(TargetActor))
		{
			continue;
		}

		FVector ToTarget = TargetActor->GetActorLocation() - SourceLocation;
		ToTarget.Z = 0.0f;
		if (ToTarget.IsNearlyZero())
		{
			ConeTargets.Add(TargetActor);
			continue;
		}

		if (FVector::DotProduct(AimDirection, ToTarget.GetSafeNormal()) >= CosHalfAngle)
		{
			ConeTargets.Add(TargetActor);
		}
	}

	if (ConeTargets.Num() == 0)
	{
		return true;
	}

	const int32 ConsumedStacks = ConsumeCombatAccelerationStacks(SourceActor);
	const float FinalAttackCoefficient = BaseAttackCoefficient + (AdditionalCoefficientPerStack * ConsumedStacks);

	int32 ValidHitCount = 0;
	for (AActor* TargetActor : ConeTargets)
	{
		if (SourceCombatComponent->ApplyDamageEffectToTarget(
			TargetActor,
			DamageEffectClass,
			1.0f,
			FixedDamage,
			FinalAttackCoefficient,
			bCanCrit,
			ResolvedBreakPower))
		{
			++ValidHitCount;
		}
	}

	if (bEnableDebugVisualization)
	{
		UE_LOG(
			LogLS,
			Log,
			TEXT("[Overclock] Source=%s Targets=%d ValidHits=%d ConsumedStacks=%d Coef=%.2f"),
			*GetNameSafe(SourceActor),
			ConeTargets.Num(),
			ValidHitCount,
			ConsumedStacks,
			FinalAttackCoefficient);
	}

	return true;
}

int32 ULSOverclockSkill::ConsumeCombatAccelerationStacks(AActor* SourceActor) const
{
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SourceActor);
	if (!ASC)
	{
		return 0;
	}

	FGameplayTagContainer BuffTags;
	BuffTags.AddTag(LSGameplayTags::Buff_CombatAcceleration);

	int32 StackCount = 0;
	for (const FActiveGameplayEffectHandle& Handle : ASC->GetActiveEffects(FGameplayEffectQuery::MakeQuery_MatchAllOwningTags(BuffTags)))
	{
		StackCount += FMath::Max(0, ASC->GetCurrentStackCount(Handle));
	}

	if (StackCount > 0)
	{
		ASC->RemoveActiveEffectsWithGrantedTags(BuffTags);
	}

	return StackCount;
}
