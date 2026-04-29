#include "GAS/Effects/LSGE_DashCooldown.h"
#include "GAS/LSCharacterAttributeSet.h"
#include "GameplayEffectAttributeCaptureDefinition.h"

ULSGE_DashCooldown::ULSGE_DashCooldown()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	// DashCooldown 어트리뷰트 값을 GE 지속시간으로 사용
	FAttributeBasedFloat AttributeBased;
	AttributeBased.BackingAttribute = FGameplayEffectAttributeCaptureDefinition(
		ULSCharacterAttributeSet::GetDashCooldownAttribute(),
		EGameplayEffectAttributeCaptureSource::Source,
		true
	);
	AttributeBased.AttributeCalculationType = EAttributeBasedFloatCalculationType::AttributeMagnitude;
	AttributeBased.Coefficient = FScalableFloat(1.0f);

	DurationMagnitude = FGameplayEffectModifierMagnitude(AttributeBased);
}
