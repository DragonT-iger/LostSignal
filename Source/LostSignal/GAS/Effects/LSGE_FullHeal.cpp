#include "GAS/Effects/LSGE_FullHeal.h"

#include "GAS/LSCombatAttributeSet.h"

ULSGE_FullHeal::ULSGE_FullHeal(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	// 대상의 MaxHealth를 캡처해 CurrentHealth에 그대로 덮어써 전량 회복한다.
	FAttributeBasedFloat MaxHealthMagnitude;
	MaxHealthMagnitude.BackingAttribute = FGameplayEffectAttributeCaptureDefinition(
		ULSCombatAttributeSet::GetMaxHealthAttribute(),
		EGameplayEffectAttributeCaptureSource::Target,
		false);

	FGameplayModifierInfo HealModifier;
	HealModifier.Attribute = ULSCombatAttributeSet::GetCurrentHealthAttribute();
	HealModifier.ModifierOp = EGameplayModOp::Override;
	HealModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(MaxHealthMagnitude);
	Modifiers.Add(HealModifier);
}
