#include "Skills/LSBypassSkillDataAsset.h"

#include "GAS/Abilities/Character1/LSGA_Bypass.h"
#include "GAS/Effects/LSGE_Stun.h"
#include "GAS/LSGameplayTags.h"
#include "Skills/LSBypassHologramActor.h"

ULSBypassSkillDataAsset::ULSBypassSkillDataAsset()
{
	AbilityClass = ULSGA_Bypass::StaticClass();
	CooldownTag = LSGameplayTags::Cooldown_Skill_Bypass;
	HologramActorClass = ALSBypassHologramActor::StaticClass();
	StunEffectClass = ULSGE_Stun::StaticClass();
}
