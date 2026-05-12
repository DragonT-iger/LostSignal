#include "GAS/Effects/LSGE_CombatAcceleration.h"

#include "GAS/LSCharacterAttributeSet.h"
#include "GAS/LSGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

ULSGE_CombatAcceleration::ULSGE_CombatAcceleration(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	StackingType = EGameplayEffectStackingType::AggregateBySource;
	StackLimitCount = 5;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;
	StackPeriodResetPolicy = EGameplayEffectStackingPeriodPolicy::ResetOnSuccessfulApplication;
	StackExpirationPolicy = EGameplayEffectStackingExpirationPolicy::ClearEntireStack;

	UTargetTagsGameplayEffectComponent* TargetTagsComp =
		ObjectInitializer.CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(this, TEXT("CombatAccelerationTargetTags"));
	FInheritedTagContainer TagContainer;
	TagContainer.Added.AddTag(LSGameplayTags::Buff_CombatAcceleration);
	TargetTagsComp->SetAndApplyTargetTagChanges(TagContainer);
	GEComponents.Add(TargetTagsComp);

	FSetByCallerFloat AttackSpeedSetByCaller;
	AttackSpeedSetByCaller.DataTag = LSGameplayTags::Data_Buff_AttackSpeed;

	FGameplayModifierInfo AttackSpeedModifier;
	AttackSpeedModifier.Attribute = ULSCharacterAttributeSet::GetAttackSpeedAttribute();
	AttackSpeedModifier.ModifierOp = EGameplayModOp::Additive;
	AttackSpeedModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(AttackSpeedSetByCaller);
	Modifiers.Add(AttackSpeedModifier);

	FSetByCallerFloat AttackSetByCaller;
	AttackSetByCaller.DataTag = LSGameplayTags::Data_Buff_AttackPower;

	FGameplayModifierInfo AttackPowerModifier;
	AttackPowerModifier.Attribute = ULSCharacterAttributeSet::GetAttackPowerMultiplierAttribute();
	AttackPowerModifier.ModifierOp = EGameplayModOp::Additive;
	AttackPowerModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(AttackSetByCaller);
	Modifiers.Add(AttackPowerModifier);
}
