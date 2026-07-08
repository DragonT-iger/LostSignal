#include "Session/LSSkillCastSettingsSubsystem.h"

void ULSSkillCastSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	LoadConfig();
}

bool ULSSkillCastSettingsSubsystem::IsSlotSmartKeyEnabled(ELSPlayerSkillSlot Slot) const
{
	switch (Slot)
	{
	case ELSPlayerSkillSlot::Skill1: return bSkill1SmartKeyEnabled;
	case ELSPlayerSkillSlot::Skill2: return bSkill2SmartKeyEnabled;
	case ELSPlayerSkillSlot::Skill3: return bSkill3SmartKeyEnabled;
	case ELSPlayerSkillSlot::Dash: break;
	}

	return false;
}

void ULSSkillCastSettingsSubsystem::SetSlotSmartKeyEnabled(ELSPlayerSkillSlot Slot, bool bEnabled)
{
	switch (Slot)
	{
	case ELSPlayerSkillSlot::Skill1: bSkill1SmartKeyEnabled = bEnabled; break;
	case ELSPlayerSkillSlot::Skill2: bSkill2SmartKeyEnabled = bEnabled; break;
	case ELSPlayerSkillSlot::Skill3: bSkill3SmartKeyEnabled = bEnabled; break;
	case ELSPlayerSkillSlot::Dash: return;
	}

	SaveConfig();
}

void ULSSkillCastSettingsSubsystem::SetSmartKeyPreviewOnReleaseEnabled(bool bEnabled)
{
	bSmartKeyPreviewOnRelease = bEnabled;
	SaveConfig();
}

ELSSkillCastMode ULSSkillCastSettingsSubsystem::GetSlotCastMode(ELSPlayerSkillSlot Slot) const
{
	if (!IsSlotSmartKeyEnabled(Slot))
	{
		return ELSSkillCastMode::PreviewConfirm;
	}

	return bSmartKeyPreviewOnRelease ? ELSSkillCastMode::QuickCastWithIndicator : ELSSkillCastMode::QuickCast;
}

void ULSSkillCastSettingsSubsystem::SetSlotCastMode(ELSPlayerSkillSlot Slot, ELSSkillCastMode Mode)
{
	switch (Mode)
	{
	case ELSSkillCastMode::PreviewConfirm:
		SetSlotSmartKeyEnabled(Slot, false);
		return;
	case ELSSkillCastMode::QuickCastWithIndicator:
		SetSlotSmartKeyEnabled(Slot, true);
		SetSmartKeyPreviewOnReleaseEnabled(true);
		return;
	case ELSSkillCastMode::QuickCast:
		SetSlotSmartKeyEnabled(Slot, true);
		SetSmartKeyPreviewOnReleaseEnabled(false);
		return;
	}
}
