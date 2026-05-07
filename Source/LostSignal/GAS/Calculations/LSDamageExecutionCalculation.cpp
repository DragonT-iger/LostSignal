#include "GAS/Calculations/LSDamageExecutionCalculation.h"

#include "AbilitySystemComponent.h"
#include "GAS/LSCharacterAttributeSet.h"
#include "GAS/LSCombatAttributeSet.h"
#include "GAS/LSGameplayTags.h"
#include "GameplayEffectExtension.h"
#include "LostSignal.h"

namespace
{
struct FLSDamageCaptureStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(Attack)
	DECLARE_ATTRIBUTE_CAPTUREDEF(CritChance)
	DECLARE_ATTRIBUTE_CAPTUREDEF(CritDamage)
	DECLARE_ATTRIBUTE_CAPTUREDEF(ArmorPenetration)
	DECLARE_ATTRIBUTE_CAPTUREDEF(Defence)

	FLSDamageCaptureStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(ULSCharacterAttributeSet, Attack, Source, false)
		DEFINE_ATTRIBUTE_CAPTUREDEF(ULSCharacterAttributeSet, CritChance, Source, false)
		DEFINE_ATTRIBUTE_CAPTUREDEF(ULSCharacterAttributeSet, CritDamage, Source, false)
		DEFINE_ATTRIBUTE_CAPTUREDEF(ULSCharacterAttributeSet, ArmorPenetration, Source, false)
		DEFINE_ATTRIBUTE_CAPTUREDEF(ULSCharacterAttributeSet, Defence, Target, false)
	}
};

const FLSDamageCaptureStatics& DamageCaptureStatics()
{
	static FLSDamageCaptureStatics Statics;
	return Statics;
}
}

ULSDamageExecutionCalculation::ULSDamageExecutionCalculation()
{
	RelevantAttributesToCapture.Add(DamageCaptureStatics().AttackDef);
	RelevantAttributesToCapture.Add(DamageCaptureStatics().CritChanceDef);
	RelevantAttributesToCapture.Add(DamageCaptureStatics().CritDamageDef);
	RelevantAttributesToCapture.Add(DamageCaptureStatics().ArmorPenetrationDef);
	RelevantAttributesToCapture.Add(DamageCaptureStatics().DefenceDef);
}

void ULSDamageExecutionCalculation::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	FAggregatorEvaluateParameters EvaluateParameters;
	EvaluateParameters.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvaluateParameters.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	const FLSDamageCaptureStatics& Statics = DamageCaptureStatics();

	float Attack = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.AttackDef, EvaluateParameters, Attack);

	float CritChance = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.CritChanceDef, EvaluateParameters, CritChance);

	float CritDamage = 1.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.CritDamageDef, EvaluateParameters, CritDamage);

	float ArmorPenetration = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.ArmorPenetrationDef, EvaluateParameters, ArmorPenetration);

	float Defence = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(Statics.DefenceDef, EvaluateParameters, Defence);

	const float BaseDamage = Spec.GetSetByCallerMagnitude(LSGameplayTags::Data_Damage_Base, false, 0.0f);
	const float AttackCoefficient = Spec.GetSetByCallerMagnitude(LSGameplayTags::Data_Damage_AttackCoefficient, false, 0.0f);
	const bool bCanCrit = Spec.GetSetByCallerMagnitude(LSGameplayTags::Data_Damage_CanCrit, false, 0.0f) > 0.0f;

	const float RawDamage = FMath::Max(0.0f, BaseDamage + (FMath::Max(0.0f, Attack) * AttackCoefficient));
	const float EffectiveDefence = FMath::Max(0.0f, Defence - ArmorPenetration);
	float FinalDamage = RawDamage * (100.0f / (100.0f + EffectiveDefence));
	const float DamageBeforeCrit = FinalDamage;

	const float ClampedCritChance = FMath::Clamp(CritChance, 0.0f, 1.0f);
	const float CritRoll = bCanCrit ? FMath::FRand() : 1.0f;
	const bool bCriticalHit = bCanCrit && CritRoll < ClampedCritChance;
	const float AppliedCritDamage = FMath::Max(1.0f, CritDamage);
	if (bCriticalHit)
	{
		FinalDamage *= AppliedCritDamage;
	}

	FinalDamage = FMath::Max(0.0f, FinalDamage);
	const UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
	const UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
	const AActor* SourceActor = SourceASC ? SourceASC->GetAvatarActor() : nullptr;
	const AActor* TargetActor = TargetASC ? TargetASC->GetAvatarActor() : nullptr;

	UE_LOG(
		LogLS,
		Log,
		TEXT("DamageCalc %s -> %s | FinalDamage = max(0, (BaseDamage %.2f + Attack %.2f * Coef %.2f) * (100 / (100 + max(0, Defence %.2f - ArmorPen %.2f)))%s) = %.2f | CritRoll %.3f / CritChance %.3f"),
		*GetNameSafe(SourceActor),
		*GetNameSafe(TargetActor),
		BaseDamage,
		Attack,
		AttackCoefficient,
		Defence,
		ArmorPenetration,
		*(bCriticalHit ? FString::Printf(TEXT(" * CritDamage %.2f"), AppliedCritDamage) : FString()),
		FinalDamage,
		CritRoll,
		ClampedCritChance);

	if (FinalDamage <= 0.0f)
	{
		return;
	}

	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
		ULSCombatAttributeSet::GetDamageAttribute(),
		EGameplayModOp::Additive,
		FinalDamage));
}
