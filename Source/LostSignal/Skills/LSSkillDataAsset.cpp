#include "Skills/LSSkillDataAsset.h"

#include "Abilities/GameplayAbility.h"
#include "Data/LSCharacterSkillRow.h"
#include "GAS/Effects/LSGE_PlayerBasicDamage.h"

ULSSkillDataAsset::ULSSkillDataAsset()
{
	DamageEffectClass = ULSGE_PlayerBasicDamage::StaticClass();
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

const FLSCharacterSkillRow* ULSSkillDataAsset::ResolveSkillRow() const
{
	return SkillRow.GetRow<FLSCharacterSkillRow>(TEXT("LSSkillDataAsset"));
}
