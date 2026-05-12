#include "Skills/LSSkillDataAsset.h"

#include "Skills/LSSkill.h"

bool ULSSkillDataAsset::ActivateSkill(const FLSSkillActivationContext& Context) const
{
	ULSSkill* Skill = GetSkillDefaultObject();
	return Skill ? Skill->ActivateSkill(Context) : false;
}

bool ULSSkillDataAsset::HandleBasicAttackHit(const FLSBasicAttackHitContext& Context) const
{
	const ULSSkill* Skill = GetSkillDefaultObject();
	return Skill ? Skill->HandleBasicAttackHit(Context) : false;
}

FLSSkillAreaPreviewSpec ULSSkillDataAsset::BuildPreviewSpec() const
{
	const ULSSkill* Skill = GetSkillDefaultObject();
	return Skill ? Skill->BuildPreviewSpec() : FLSSkillAreaPreviewSpec();
}

bool ULSSkillDataAsset::TryGetSkillRow(FLSCharacterSkillRow& OutRow) const
{
	const ULSSkill* Skill = GetSkillDefaultObject();
	return Skill ? Skill->TryGetSkillRow(OutRow) : false;
}

ULSSkill* ULSSkillDataAsset::GetSkillDefaultObject() const
{
	return SkillClass ? SkillClass->GetDefaultObject<ULSSkill>() : nullptr;
}
