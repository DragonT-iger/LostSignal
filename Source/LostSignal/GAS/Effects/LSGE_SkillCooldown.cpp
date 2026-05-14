#include "GAS/Effects/LSGE_SkillCooldown.h"

ULSGE_SkillCooldown::ULSGE_SkillCooldown(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FScalableFloat(1.0f);
}
