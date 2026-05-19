#include "Skills/LSExecutionSkillDataAsset.h"

#include "GAS/Abilities/Character1/LSGA_Execution.h"
#include "GAS/LSGameplayTags.h"

ULSExecutionSkillDataAsset::ULSExecutionSkillDataAsset()
{
	AbilityClass = ULSGA_Execution::StaticClass();
	CooldownTag = LSGameplayTags::Cooldown_Skill_Execution;
	AttackCoefficient = 3.0f;
	BreakPower = ELSBreakPowerTier::HardCrowdControl;
}
