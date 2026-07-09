#include "UI/Settings/LSControlSettingsWidget.h"

#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "InputCoreTypes.h"
#include "LostSignal.h"
#include "Session/LSSkillCastSettingsSubsystem.h"
#include "Skills/LSSkillTypes.h"
#include "UI/Settings/LSControlSkillRowWidget.h"

#define LOCTEXT_NAMESPACE "LSControlSettings"

void ULSControlSettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	LogMissingBindings();
	BindWidgetEvents();
	InitializeCommonSettings();
	InitializeSkillRows();

	SetIsFocusable(true);
	SetKeyboardFocus();
}

void ULSControlSettingsWidget::NativeDestruct()
{
	UnbindWidgetEvents();
	Super::NativeDestruct();
}

FReply ULSControlSettingsWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		CloseControlSettings();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void ULSControlSettingsWidget::CloseControlSettings()
{
	OnClosed.Broadcast();
	RemoveFromParent();
}

void ULSControlSettingsWidget::HandleBackClicked()
{
	CloseControlSettings();
}

void ULSControlSettingsWidget::HandleSmartKeyPreviewCheckChanged(bool bIsChecked)
{
	if (bRefreshingCommonCheckBox)
	{
		return;
	}

	if (ULSSkillCastSettingsSubsystem* Settings = GetCastSettingsSubsystem())
	{
		Settings->SetSmartKeyPreviewOnReleaseEnabled(bIsChecked);
	}
}

void ULSControlSettingsWidget::InitializeSkillRows()
{
	if (Skill1Row)
	{
		Skill1Row->InitializeRow(ELSPlayerSkillSlot::Skill1, LOCTEXT("Skill1", "스킬 1"));
	}
	if (Skill2Row)
	{
		Skill2Row->InitializeRow(ELSPlayerSkillSlot::Skill2, LOCTEXT("Skill2", "스킬 2"));
	}
	if (Skill3Row)
	{
		Skill3Row->InitializeRow(ELSPlayerSkillSlot::Skill3, LOCTEXT("Skill3", "스킬 3"));
	}
	if (Skill4Row)
	{
		Skill4Row->InitializeRow(ELSPlayerSkillSlot::Skill4, LOCTEXT("Skill4", "스킬 4"));
	}
}

void ULSControlSettingsWidget::InitializeCommonSettings()
{
	if (!SmartKeyPreviewCheckBox)
	{
		return;
	}

	const ULSSkillCastSettingsSubsystem* Settings = GetCastSettingsSubsystem();
	const bool bPreviewOnRelease = Settings ? Settings->IsSmartKeyPreviewOnReleaseEnabled() : true;

	bRefreshingCommonCheckBox = true;
	SmartKeyPreviewCheckBox->SetIsChecked(bPreviewOnRelease);
	bRefreshingCommonCheckBox = false;
}

void ULSControlSettingsWidget::BindWidgetEvents()
{
	if (BackButton)
	{
		BackButton->OnClicked.AddDynamic(this, &ULSControlSettingsWidget::HandleBackClicked);
	}
	if (SmartKeyPreviewCheckBox)
	{
		SmartKeyPreviewCheckBox->OnCheckStateChanged.AddDynamic(this, &ULSControlSettingsWidget::HandleSmartKeyPreviewCheckChanged);
	}
}

void ULSControlSettingsWidget::UnbindWidgetEvents()
{
	if (BackButton)
	{
		BackButton->OnClicked.RemoveDynamic(this, &ULSControlSettingsWidget::HandleBackClicked);
	}
	if (SmartKeyPreviewCheckBox)
	{
		SmartKeyPreviewCheckBox->OnCheckStateChanged.RemoveDynamic(this, &ULSControlSettingsWidget::HandleSmartKeyPreviewCheckChanged);
	}
}

void ULSControlSettingsWidget::LogMissingBindings() const
{
	if (!BackButton)
	{
		UE_LOG(LogLS, Warning, TEXT("BackButton is not bound on %s."), *GetNameSafe(this));
	}
	if (!SmartKeyPreviewCheckBox)
	{
		UE_LOG(LogLS, Warning, TEXT("SmartKeyPreviewCheckBox is not bound on %s."), *GetNameSafe(this));
	}
	if (!Skill1Row)
	{
		UE_LOG(LogLS, Warning, TEXT("Skill1Row is not bound on %s."), *GetNameSafe(this));
	}
	if (!Skill2Row)
	{
		UE_LOG(LogLS, Warning, TEXT("Skill2Row is not bound on %s."), *GetNameSafe(this));
	}
	if (!Skill3Row)
	{
		UE_LOG(LogLS, Warning, TEXT("Skill3Row is not bound on %s."), *GetNameSafe(this));
	}
	if (!Skill4Row)
	{
		UE_LOG(LogLS, Warning, TEXT("Skill4Row is not bound on %s."), *GetNameSafe(this));
	}
}

ULSSkillCastSettingsSubsystem* ULSControlSettingsWidget::GetCastSettingsSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return nullptr;
	}

	return GameInstance->GetSubsystem<ULSSkillCastSettingsSubsystem>();
}

#undef LOCTEXT_NAMESPACE
