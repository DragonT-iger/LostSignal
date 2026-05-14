#include "Skills/LSExecutionSkillDataAsset.h"

#include "GAS/Abilities/LSGA_Execution.h"

ULSExecutionSkillDataAsset::ULSExecutionSkillDataAsset()
{
	AbilityClass = ULSGA_Execution::StaticClass();
	AttackCoefficient = 3.0f;
	BreakPower = ELSBreakPowerTier::HardCrowdControl;
}
