#include "Session/LSSkillCastSettingsSubsystem.h"

ELSSkillCastMode ULSSkillCastSettingsSubsystem::GetSlotCastMode(ELSPlayerSkillSlot Slot) const
{
	switch (Slot)
	{
	case ELSPlayerSkillSlot::Skill1: return Skill1CastMode;
	case ELSPlayerSkillSlot::Skill2: return Skill2CastMode;
	case ELSPlayerSkillSlot::Skill3: return Skill3CastMode;
	case ELSPlayerSkillSlot::Dash: break; // 대쉬는 캐스트 모드가 없다(표시 전용). 기본값으로 처리.
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
	case ELSPlayerSkillSlot::Dash: return; // 대쉬는 캐스트 모드가 없다(표시 전용). 저장 없이 무시.
	}

	SaveConfig();
}
