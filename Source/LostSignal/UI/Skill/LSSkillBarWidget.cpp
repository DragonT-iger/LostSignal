#include "UI/Skill/LSSkillBarWidget.h"

#include "LostSignal.h"
#include "Skills/LSSkillTypes.h"
#include "UI/Skill/LSSkillSlotWidget.h"

void ULSSkillBarWidget::InitializeSkillBar(ULSPlayerSkillComponent* InSkillComponent)
{
	SkillComponent = InSkillComponent;
	if (!SkillComponent)
	{
		UE_LOG(LogLS, Warning, TEXT("%s cannot initialize skill bar because SkillComponent is missing."), *GetNameSafe(this));
		return;
	}

	if (Skill1Slot)
	{
		Skill1Slot->InitializeSlot(SkillComponent, ELSPlayerSkillSlot::Skill1);
	}

	if (Skill2Slot)
	{
		Skill2Slot->InitializeSlot(SkillComponent, ELSPlayerSkillSlot::Skill2);
	}

	if (Skill3Slot)
	{
		Skill3Slot->InitializeSlot(SkillComponent, ELSPlayerSkillSlot::Skill3);
	}

	if (Skill4Slot)
	{
		Skill4Slot->InitializeSlot(SkillComponent, ELSPlayerSkillSlot::Skill4);
	}

	if (UltimateSlot)
	{
		UltimateSlot->InitializeSlot(SkillComponent, ELSPlayerSkillSlot::Ultimate);
	}
}

void ULSSkillBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!Skill1Slot || !Skill2Slot || !Skill3Slot || !Skill4Slot || !UltimateSlot)
	{
		UE_LOG(LogLS, Warning, TEXT("%s is missing required skill bar slot binding. Skill1=%s Skill2=%s Skill3=%s Skill4=%s Ultimate=%s"),
			*GetNameSafe(this),
			*GetNameSafe(Skill1Slot),
			*GetNameSafe(Skill2Slot),
			*GetNameSafe(Skill3Slot),
			*GetNameSafe(Skill4Slot),
			*GetNameSafe(UltimateSlot));
	}
}
