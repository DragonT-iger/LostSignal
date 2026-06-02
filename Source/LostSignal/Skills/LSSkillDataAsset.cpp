#include "Skills/LSSkillDataAsset.h"

#include "Abilities/GameplayAbility.h"
#include "Data/LSCharacterSkillRow.h"
#include "GAS/Abilities/Character1/LSGA_Bypass.h"
#include "GAS/Abilities/Character1/LSGA_Execution.h"
#include "GAS/Abilities/Character1/LSGA_Overclock.h"
#include "GAS/Abilities/Character1/LSGA_Override.h"
#include "GAS/Abilities/Character1/LSGA_ShortCircuit.h"
#include "GAS/Effects/LSGE_SkillCooldown.h"
#include "GAS/Effects/LSGE_PlayerBasicDamage.h"
#include "GAS/LSGameplayTags.h"

namespace
{
	void ApplySkillRowRangeToPreviewSpec(const FLSCharacterSkillRow& Row, FLSSkillAreaPreviewSpec& InOutSpec)
	{
		switch (Row.Range_Shape)
		{
		case ELSCharacterSkillRangeShape::Cone:
			InOutSpec.Shape = ELSSkillAreaShape::Circle;
			InOutSpec.LocationMode = ELSSkillPreviewLocationMode::CasterOrigin;
			InOutSpec.Radius = Row.Range_X;
			InOutSpec.Degrees = Row.Range_Y;
			break;

		case ELSCharacterSkillRangeShape::Circle:
			InOutSpec.Shape = ELSSkillAreaShape::Circle;
			InOutSpec.Radius = Row.Range_X;
			InOutSpec.Degrees = 360.0f;
			break;

		case ELSCharacterSkillRangeShape::Box:
			InOutSpec.Shape = ELSSkillAreaShape::Box;
			InOutSpec.LocationMode = ELSSkillPreviewLocationMode::CasterOrigin;
			InOutSpec.BoxLength = Row.Range_X;
			InOutSpec.BoxWidth = Row.Range_Y;
			if (InOutSpec.LocationOffset.IsNearlyZero() && Row.Range_X > 0.0f)
			{
				InOutSpec.LocationOffset.X = Row.Range_X * 0.5f;
			}
			break;

		case ELSCharacterSkillRangeShape::None:
		default:
			break;
		}

		InOutSpec.WorldZOffset = 0;
	}
}

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

	ApplySkillRowRangeToPreviewSpec(*Row, ResolvedSpec);
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
