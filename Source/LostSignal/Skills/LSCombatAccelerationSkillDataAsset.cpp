#include "Skills/LSCombatAccelerationSkillDataAsset.h"

#include "GAS/Abilities/Character1/LSGA_CombatAccelerationPassive.h"
#include "GAS/Effects/LSGE_CombatAcceleration.h"

ULSCombatAccelerationSkillDataAsset::ULSCombatAccelerationSkillDataAsset()
{
	AbilityClass = ULSGA_CombatAccelerationPassive::StaticClass();
	FallbackCooldown = 0.0f;
	BuffEffectClass = ULSGE_CombatAcceleration::StaticClass();
}
