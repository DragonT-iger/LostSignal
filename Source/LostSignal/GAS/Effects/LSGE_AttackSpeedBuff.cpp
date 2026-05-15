#include "GAS/Effects/LSGE_AttackSpeedBuff.h"

#include "GAS/LSCharacterAttributeSet.h"
#include "GAS/LSGameplayTags.h"

ULSGE_AttackSpeedBuff::ULSGE_AttackSpeedBuff(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FSetByCallerFloat AttackSpeedSetByCaller;
	AttackSpeedSetByCaller.DataTag = LSGameplayTags::Data_Buff_AttackSpeed;

	FGameplayModifierInfo AttackSpeedModifier;
	AttackSpeedModifier.Attribute = ULSCharacterAttributeSet::GetAttackSpeedAttribute();
	AttackSpeedModifier.ModifierOp = EGameplayModOp::Additive;
	AttackSpeedModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(AttackSpeedSetByCaller);
	Modifiers.Add(AttackSpeedModifier);
}
