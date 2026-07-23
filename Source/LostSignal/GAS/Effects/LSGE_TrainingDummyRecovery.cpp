#include "GAS/Effects/LSGE_TrainingDummyRecovery.h"

#include "GAS/LSCombatAttributeSet.h"
#include "GAS/LSGameplayTags.h"

ULSGE_TrainingDummyRecovery::ULSGE_TrainingDummyRecovery(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period.SetValue(1.0f);

	FSetByCallerFloat RecoverySetByCaller;
	RecoverySetByCaller.DataTag = LSGameplayTags::Data_Health_Amount;

	FGameplayModifierInfo RecoveryModifier;
	RecoveryModifier.Attribute = ULSCombatAttributeSet::GetCurrentHealthAttribute();
	RecoveryModifier.ModifierOp = EGameplayModOp::Additive;
	RecoveryModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(RecoverySetByCaller);
	Modifiers.Add(RecoveryModifier);
}
