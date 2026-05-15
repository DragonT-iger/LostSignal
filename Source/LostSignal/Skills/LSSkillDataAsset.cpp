#include "Skills/LSSkillDataAsset.h"

#include "Abilities/GameplayAbility.h"
#include "Data/LSCharacterSkillRow.h"
#include "GAS/Abilities/LSGA_Bypass.h"
#include "GAS/Abilities/LSGA_Execution.h"
#include "GAS/Abilities/LSGA_Overclock.h"
#include "GAS/Abilities/LSGA_Override.h"
#include "GAS/Abilities/LSGA_ShortCircuit.h"
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
	FLSSkillAreaPreviewSpec ResolvedSpec = PreviewSpec;
	const FLSCharacterSkillRow* Row = ResolveSkillRow();
	if (!Row)
	{
		return ResolvedSpec;
	}

	switch (Row->Range_Shape)
	{
	case ELSCharacterSkillRangeShape::Cone:
		ResolvedSpec.Shape = ELSSkillAreaShape::Circle;
		ResolvedSpec.LocationMode = ELSSkillPreviewLocationMode::CasterOrigin;
		ResolvedSpec.Radius = Row->Range_X;
		ResolvedSpec.Degrees = Row->Range_Y > 0.0f ? Row->Range_Y : ResolvedSpec.Degrees;
		break;

	case ELSCharacterSkillRangeShape::Circle:
		ResolvedSpec.Shape = ELSSkillAreaShape::Circle;
		ResolvedSpec.Radius = Row->Range_X;
		ResolvedSpec.Degrees = 360.0f;
		break;

	case ELSCharacterSkillRangeShape::Box:
		ResolvedSpec.Shape = ELSSkillAreaShape::Box;
		ResolvedSpec.LocationMode = ELSSkillPreviewLocationMode::CasterOrigin;
		ResolvedSpec.BoxLength = Row->Range_X;
		ResolvedSpec.BoxWidth = Row->Range_Y;
		if (ResolvedSpec.LocationOffset.IsNearlyZero() && Row->Range_X > 0.0f)
		{
			ResolvedSpec.LocationOffset.X = Row->Range_X * 0.5f;
		}
		break;

	default:
		break;
	}

	if (Row->Range_Z > 0.0f)
	{
		ResolvedSpec.WorldZOffset = Row->Range_Z;
	}

	return ResolvedSpec;
}

bool ULSSkillDataAsset::TryGetSkillRow(FLSCharacterSkillRow& OutRow) const
{
	if (const FLSCharacterSkillRow* Row = ResolveSkillRow())
	{
		OutRow = *Row;
		return true;
	}

	return false;
}

TSubclassOf<UGameplayAbility> ULSSkillDataAsset::GetAbilityClass() const
{
	return AbilityClass;
}

float ULSSkillDataAsset::GetCooldownDuration() const
{
	FLSCharacterSkillRow Row;
	if (TryGetSkillRow(Row) && Row.Skill_Cooldown > 0.0f)
	{
		return Row.Skill_Cooldown;
	}

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

ULSSkillDataAsset* ULSSkillDataAsset::GetEnhancementVariant(int32 Index) const
{
	return EnhancementVariants.IsValidIndex(Index) ? EnhancementVariants[Index].Get() : nullptr;
}

const FLSCharacterSkillRow* ULSSkillDataAsset::ResolveSkillRow() const
{
	return SkillRow.GetRow<FLSCharacterSkillRow>(TEXT("LSSkillDataAsset"));
}
