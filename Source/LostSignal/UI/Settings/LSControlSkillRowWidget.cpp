#include "UI/Settings/LSControlSkillRowWidget.h"

#include "Components/CheckBox.h"
#include "Components/TextBlock.h"
#include "LostSignal.h"
#include "Session/LSSkillCastSettingsSubsystem.h"

void ULSControlSkillRowWidget::NativeConstruct()
{
	Super::NativeConstruct();

	LogMissingBindings();
	BindCheckBoxEvents();
}

void ULSControlSkillRowWidget::NativeDestruct()
{
	UnbindCheckBoxEvents();
	Super::NativeDestruct();
}

void ULSControlSkillRowWidget::InitializeRow(ELSPlayerSkillSlot InSlot, const FText& InSlotLabel)
{
	Slot = InSlot;

	if (SlotNameText)
	{
		SlotNameText->SetText(InSlotLabel);
	}

	const ULSSkillCastSettingsSubsystem* Settings = GetCastSettingsSubsystem();
	RefreshSmartKeyCheck(Settings ? Settings->IsSlotSmartKeyEnabled(Slot) : true);
}

void ULSControlSkillRowWidget::HandleSmartKeyCheckChanged(bool bIsChecked)
{
	if (bRefreshingCheckBox)
	{
		return;
	}

	if (ULSSkillCastSettingsSubsystem* Settings = GetCastSettingsSubsystem())
	{
		Settings->SetSlotSmartKeyEnabled(Slot, bIsChecked);
	}

	OnSmartKeyChanged(bIsChecked);
}

void ULSControlSkillRowWidget::RefreshSmartKeyCheck(bool bEnabled)
{
	if (!SmartKeyCheckBox)
	{
		return;
	}

	bRefreshingCheckBox = true;
	SmartKeyCheckBox->SetIsChecked(bEnabled);
	bRefreshingCheckBox = false;
}

void ULSControlSkillRowWidget::BindCheckBoxEvents()
{
	if (SmartKeyCheckBox)
	{
		SmartKeyCheckBox->OnCheckStateChanged.AddDynamic(this, &ULSControlSkillRowWidget::HandleSmartKeyCheckChanged);
	}
}

void ULSControlSkillRowWidget::UnbindCheckBoxEvents()
{
	if (SmartKeyCheckBox)
	{
		SmartKeyCheckBox->OnCheckStateChanged.RemoveDynamic(this, &ULSControlSkillRowWidget::HandleSmartKeyCheckChanged);
	}
}

void ULSControlSkillRowWidget::LogMissingBindings() const
{
	if (!SlotNameText)
	{
		UE_LOG(LogLS, Warning, TEXT("SlotNameText is not bound on %s."), *GetNameSafe(this));
	}
	if (!SmartKeyCheckBox)
	{
		UE_LOG(LogLS, Warning, TEXT("SmartKeyCheckBox is not bound on %s."), *GetNameSafe(this));
	}
}

ULSSkillCastSettingsSubsystem* ULSControlSkillRowWidget::GetCastSettingsSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return nullptr;
	}

	return GameInstance->GetSubsystem<ULSSkillCastSettingsSubsystem>();
}
