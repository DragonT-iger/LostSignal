#include "Skills/LSSkillDataAsset.h"

#include "Abilities/GameplayAbility.h"
#include "GAS/Abilities/Character1/LSGA_Bypass.h"
#include "GAS/Abilities/Character1/LSGA_Execution.h"
#include "GAS/Abilities/Character1/LSGA_Overclock.h"
#include "GAS/Abilities/Character1/LSGA_Override.h"
#include "GAS/Abilities/Character1/LSGA_ShortCircuit.h"
#include "GAS/Effects/LSGE_SkillCooldown.h"
#include "GAS/Effects/LSGE_PlayerBasicDamage.h"
#include "GAS/LSGameplayTags.h"

ULSSkillDataAsset::ULSSkillDataAsset()
{
	DamageEffectClass = ULSGE_PlayerBasicDamage::StaticClass();
	CooldownEffectClass = ULSGE_SkillCooldown::StaticClass();
}

FLSSkillAreaPreviewSpec ULSSkillDataAsset::BuildPreviewSpec() const
{
	return PreviewSpec;
}

TSubclassOf<UGameplayAbility> ULSSkillDataAsset::GetAbilityClass() const
{
	return AbilityClass;
}

float ULSSkillDataAsset::GetCooldownDuration() const
{
	return FallbackCooldown;
}

FGameplayTag ULSSkillDataAsset::GetCooldownTag() const
{
	if (CooldownTag.IsValid())
	{
		return CooldownTag;
	}

	if (AbilityClass == ULSGA_Overclock::StaticClass())
	{
		return LSGameplayTags::Cooldown_Skill_Overclock;
	}

	if (AbilityClass == ULSGA_Bypass::StaticClass())
	{
		return LSGameplayTags::Cooldown_Skill_Bypass;
	}

	if (AbilityClass == ULSGA_ShortCircuit::StaticClass())
	{
		return LSGameplayTags::Cooldown_Skill_ShortCircuit;
	}

	if (AbilityClass == ULSGA_Override::StaticClass())
	{
		return LSGameplayTags::Cooldown_Skill_Override;
	}

	if (AbilityClass == ULSGA_Execution::StaticClass())
	{
		return LSGameplayTags::Cooldown_Skill_Execution;
	}

	return FGameplayTag();
}

int32 ULSSkillDataAsset::GetSkillID() const
{
	return Skill_ID;
}

FName ULSSkillDataAsset::GetSkillRowName() const
{
	return Skill_ID > 0 ? FName(*FString::FromInt(Skill_ID)) : NAME_None;
}

ULSSkillDataAsset* ULSSkillDataAsset::GetEnhancementVariant(int32 Index) const
{
	return EnhancementVariants.IsValidIndex(Index) ? EnhancementVariants[Index].Get() : nullptr;
}
