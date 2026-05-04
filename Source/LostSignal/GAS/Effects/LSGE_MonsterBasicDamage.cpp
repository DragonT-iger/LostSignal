#include "GAS/Effects/LSGE_MonsterBasicDamage.h"

#include "GAS/LSCharacterAttributeSet.h"

ULSGE_MonsterBasicDamage::ULSGE_MonsterBasicDamage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayModifierInfo DamageModifier;
	DamageModifier.Attribute = ULSCharacterAttributeSet::GetCurrentHealthAttribute();
	DamageModifier.ModifierOp = EGameplayModOp::Additive;
	DamageModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(-10.0f));
	Modifiers.Add(DamageModifier);
}
