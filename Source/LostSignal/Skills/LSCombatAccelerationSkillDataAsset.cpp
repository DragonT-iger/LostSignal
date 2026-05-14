#include "Skills/LSCombatAccelerationSkillDataAsset.h"

#include "GAS/Abilities/LSGA_CombatAccelerationPassive.h"
#include "GAS/Effects/LSGE_CombatAcceleration.h"

ULSCombatAccelerationSkillDataAsset::ULSCombatAccelerationSkillDataAsset()
{
	AbilityClass = ULSGA_CombatAccelerationPassive::StaticClass();
	BuffEffectClass = ULSGE_CombatAcceleration::StaticClass();
}
