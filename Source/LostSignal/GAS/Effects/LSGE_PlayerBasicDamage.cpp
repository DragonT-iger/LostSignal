#include "GAS/Effects/LSGE_PlayerBasicDamage.h"

#include "GAS/LSCombatAttributeSet.h"

ULSGE_PlayerBasicDamage::ULSGE_PlayerBasicDamage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo DamageModifier;
	DamageModifier.Attribute = ULSCombatAttributeSet::GetCurrentHealthAttribute();
	DamageModifier.ModifierOp = EGameplayModOp::Additive;
	DamageModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-15.0f));
	Modifiers.Add(DamageModifier);
}
