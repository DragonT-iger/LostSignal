#include "Skills/LSOverrideSkillDataAsset.h"

#include "GAS/Abilities/LSGA_Override.h"
#include "GAS/Effects/LSGE_AttackSpeedBuff.h"
#include "GAS/LSGameplayTags.h"

ULSOverrideSkillDataAsset::ULSOverrideSkillDataAsset()
{
	AbilityClass = ULSGA_Override::StaticClass();
	CooldownTag = LSGameplayTags::Cooldown_Skill_Override;
	AttackSpeedBuffEffectClass = ULSGE_AttackSpeedBuff::StaticClass();
}
