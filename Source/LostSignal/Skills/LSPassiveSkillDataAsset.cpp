#include "Skills/LSPassiveSkillDataAsset.h"

ULSPassiveSkillDataAsset::ULSPassiveSkillDataAsset()
{
}

int32 ULSPassiveSkillDataAsset::GetPassiveSkillID() const
{
	return PassiveSkill_ID;
}

FName ULSPassiveSkillDataAsset::GetPassiveSkillRowName() const
{
	return PassiveSkill_ID > 0 ? FName(*FString::FromInt(PassiveSkill_ID)) : NAME_None;
}
