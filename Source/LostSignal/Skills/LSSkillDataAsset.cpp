#include "Skills/LSSkillDataAsset.h"

#include "Abilities/GameplayAbility.h"
#include "Data/LSCharacterSkillRow.h"
#include "Data/LSGameDataSubsystem.h"
#include "Engine/GameInstance.h"
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

FLSSkillAreaPreviewSpec ULSSkillDataAsset::BuildPreviewSpecForWorld(const UObject* WorldContextObject) const
{
	FLSSkillAreaPreviewSpec ResolvedSpec = PreviewSpec;
	const FLSCharacterSkillRow* Row = ResolveSkillRowForWorld(WorldContextObject);
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

bool ULSSkillDataAsset::TryGetSkillRowForWorld(const UObject* WorldContextObject, FLSCharacterSkillRow& OutRow) const
{
	if (const FLSCharacterSkillRow* Row = ResolveSkillRowForWorld(WorldContextObject))
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

float ULSSkillDataAsset::GetCooldownDurationForWorld(const UObject* WorldContextObject) const
{
	FLSCharacterSkillRow Row;
	if (TryGetSkillRowForWorld(WorldContextObject, Row) && Row.Skill_Cooldown > 0.0f)
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

FName ULSSkillDataAsset::GetSkillRowName() const
{
	return SkillRow.RowName;
}

ULSSkillDataAsset* ULSSkillDataAsset::GetEnhancementVariant(int32 Index) const
{
	return EnhancementVariants.IsValidIndex(Index) ? EnhancementVariants[Index].Get() : nullptr;
}

const FLSCharacterSkillRow* ULSSkillDataAsset::ResolveSkillRow() const
{
	const FLSCharacterSkillRow* Row = SkillRow.GetRow<FLSCharacterSkillRow>(TEXT("LSSkillDataAsset"));
	if (Row)
	{
		const_cast<FLSCharacterSkillRow*>(Row)->NormalizeSkillIDFromRowName(SkillRow.RowName);
	}

	return Row;
}

const FLSCharacterSkillRow* ULSSkillDataAsset::ResolveSkillRowForWorld(const UObject* WorldContextObject) const
{
	if (WorldContextObject)
	{
		if (const UWorld* World = WorldContextObject->GetWorld())
		{
			if (UGameInstance* GameInstance = World->GetGameInstance())
			{
				if (const ULSGameDataSubsystem* GameDataSubsystem = GameInstance->GetSubsystem<ULSGameDataSubsystem>())
				{
					if (const FLSCharacterSkillRow* Row = GameDataSubsystem->FindActiveSkillRow(GetSkillRowName(), TEXT("LSSkillDataAsset")))
					{
						return Row;
					}
				}
			}
		}
	}

	return ResolveSkillRow();
}
