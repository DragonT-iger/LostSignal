#include "Session/LSSkillCastSettingsSubsystem.h"

ELSSkillCastMode ULSSkillCastSettingsSubsystem::GetSlotCastMode(ELSPlayerSkillSlot Slot) const
{
	switch (Slot)
	{
	case ELSPlayerSkillSlot::Skill1: return Skill1CastMode;
	case ELSPlayerSkillSlot::Skill2: return Skill2CastMode;
	case ELSPlayerSkillSlot::Skill3: return Skill3CastMode;
	case ELSPlayerSkillSlot::Skill4: return Skill4CastMode;
	case ELSPlayerSkillSlot::Ultimate: return UltimateCastMode;
	}
	return ELSSkillCastMode::PreviewConfirm;
}

void ULSSkillCastSettingsSubsystem::SetSlotCastMode(ELSPlayerSkillSlot Slot, ELSSkillCastMode Mode)
{
	switch (Slot)
	{
	case ELSPlayerSkillSlot::Skill1: Skill1CastMode = Mode; break;
	case ELSPlayerSkillSlot::Skill2: Skill2CastMode = Mode; break;
	case ELSPlayerSkillSlot::Skill3: Skill3CastMode = Mode; break;
	case ELSPlayerSkillSlot::Skill4: Skill4CastMode = Mode; break;
	case ELSPlayerSkillSlot::Ultimate: UltimateCastMode = Mode; break;
	}

	SaveConfig();
}
