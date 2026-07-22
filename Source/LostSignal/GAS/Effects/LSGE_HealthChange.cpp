#include "GAS/Effects/LSGE_HealthChange.h"

#include "GAS/LSCombatAttributeSet.h"
#include "GAS/LSGameplayTags.h"

ULSGE_HealthChange::ULSGE_HealthChange(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FSetByCallerFloat HealthAmountSetByCaller;
	HealthAmountSetByCaller.DataTag = LSGameplayTags::Data_Health_Amount;

	FGameplayModifierInfo HealthModifier;
	HealthModifier.Attribute = ULSCombatAttributeSet::GetCurrentHealthAttribute();
	HealthModifier.ModifierOp = EGameplayModOp::Additive;
	HealthModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(HealthAmountSetByCaller);
	Modifiers.Add(HealthModifier);
}
