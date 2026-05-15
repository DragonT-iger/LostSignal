#include "Skills/LSBypassSkillDataAsset.h"

#include "GAS/Abilities/LSGA_Bypass.h"
#include "GAS/LSGameplayTags.h"

ULSBypassSkillDataAsset::ULSBypassSkillDataAsset()
{
	AbilityClass = ULSGA_Bypass::StaticClass();
	CooldownTag = LSGameplayTags::Cooldown_Skill_Bypass;
}
