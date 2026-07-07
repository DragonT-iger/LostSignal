#include "Skills/LSSkillPoolDataAsset.h"

#include "Skills/LSSkillDataAsset.h"

ULSSkillDataAsset* ULSSkillPoolDataAsset::FindSkillByID(const int32 SkillID) const
{
	if (SkillID == 0)
	{
		return nullptr;
	}

	for (const TObjectPtr<ULSSkillDataAsset>& SkillData : SelectableSkills)
	{
		if (SkillData && SkillData->GetSkillID() == SkillID)
		{
			return SkillData;
		}
	}

	return nullptr;
}
