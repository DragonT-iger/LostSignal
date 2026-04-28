#include "GAS/Effects/LSGE_DashCooldown.h"

ULSGE_DashCooldown::ULSGE_DashCooldown()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(0.3f));
}
