#include "GAS/Effects/LSGE_StaminaChange.h"

#include "GAS/LSCharacterAttributeSet.h"
#include "GAS/LSGameplayTags.h"

ULSGE_StaminaChange::ULSGE_StaminaChange(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FSetByCallerFloat StaminaAmountSetByCaller;
	StaminaAmountSetByCaller.DataTag = LSGameplayTags::Data_Stamina_Amount;

	FGameplayModifierInfo StaminaModifier;
	StaminaModifier.Attribute = ULSCharacterAttributeSet::GetCurrentStaminaAttribute();
	StaminaModifier.ModifierOp = EGameplayModOp::Additive;
	StaminaModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(StaminaAmountSetByCaller);
	Modifiers.Add(StaminaModifier);
}
