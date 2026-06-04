#include "Skills/LSSkillDataAssetBase.h"

#include "GAS/Effects/LSGE_SkillCooldown.h"

ULSSkillDataAssetBase::ULSSkillDataAssetBase()
{
	CooldownEffectClass = ULSGE_SkillCooldown::StaticClass();
}

TSubclassOf<UGameplayAbility> ULSSkillDataAssetBase::GetAbilityClass() const
{
	return AbilityClass;
}

float ULSSkillDataAssetBase::GetCooldownDuration() const
{
	return FallbackCooldown;
}

FGameplayTag ULSSkillDataAssetBase::GetCooldownTag() const
{
	return CooldownTag;
}
