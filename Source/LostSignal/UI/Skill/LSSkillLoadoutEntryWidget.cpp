#include "UI/Skill/LSSkillLoadoutEntryWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Data/LSCharacterSkillRow.h"
#include "Data/LSGameDataSubsystem.h"
#include "Engine/GameInstance.h"
#include "LostSignal.h"
#include "Skills/LSSkillDataAsset.h"

#define LOCTEXT_NAMESPACE "LSSkillLoadoutEntryWidget"

void ULSSkillLoadoutEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (SelectButton)
	{
		SelectButton->OnClicked.AddDynamic(this, &ULSSkillLoadoutEntryWidget::HandleSelectButtonClicked);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("SelectButton is not bound on %s."), *GetNameSafe(this));
	}

	RefreshDisplay();
	RefreshButtonState();
}

void ULSSkillLoadoutEntryWidget::NativeDestruct()
{
	if (SelectButton)
	{
		SelectButton->OnClicked.RemoveDynamic(this, &ULSSkillLoadoutEntryWidget::HandleSelectButtonClicked);
	}

	Super::NativeDestruct();
}

void ULSSkillLoadoutEntryWidget::SetSkillData(ULSSkillDataAsset* InSkillData)
{
	SkillData = InSkillData;
	RefreshDisplay();
	RefreshButtonState();
}

void ULSSkillLoadoutEntryWidget::SetDisplayOnly(const bool bInDisplayOnly)
{
	bDisplayOnly = bInDisplayOnly;
	RefreshButtonState();
}

void ULSSkillLoadoutEntryWidget::SetNamePrefix(const FText& InNamePrefix)
{
	NamePrefixText = InNamePrefix;
	RefreshDisplay();
}

void ULSSkillLoadoutEntryWidget::SetEmptyDisplayText(const FText& InNameText, const FText& InDescriptionText)
{
	SkillData = nullptr;
	EmptyNameText = InNameText;
	EmptyDescriptionText = InDescriptionText;
	RefreshDisplay();
	RefreshButtonState();
}

void ULSSkillLoadoutEntryWidget::HandleSelectButtonClicked()
{
	if (bDisplayOnly || !SkillData)
	{
		return;
	}

	OnEntryClicked.ExecuteIfBound(SkillData->GetSkillID());
}

void ULSSkillLoadoutEntryWidget::RefreshDisplay()
{
	RefreshIcon();

	const FLSCharacterSkillRow* Row = ResolveSkillRow();
	RefreshName(Row);
	RefreshDescription(Row);
}

void ULSSkillLoadoutEntryWidget::RefreshButtonState()
{
	if (SelectButton)
	{
		const bool bCanClick = !bDisplayOnly && SkillData != nullptr;
		SelectButton->SetIsEnabled(true);
		SelectButton->SetVisibility(bCanClick ? ESlateVisibility::Visible : ESlateVisibility::HitTestInvisible);
	}
}

void ULSSkillLoadoutEntryWidget::RefreshIcon()
{
	if (IconImage)
	{
		if (SkillData && SkillData->Icon)
		{
			IconImage->SetBrushFromTexture(SkillData->Icon);
			IconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			IconImage->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

const FLSCharacterSkillRow* ULSSkillLoadoutEntryWidget::ResolveSkillRow() const
{
	if (!SkillData)
	{
		return nullptr;
	}

	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const ULSGameDataSubsystem* GameDataSubsystem = GameInstance->GetSubsystem<ULSGameDataSubsystem>())
		{
			return GameDataSubsystem->FindActiveSkillRowByID(SkillData->GetSkillID(), TEXT("LSSkillLoadoutEntryWidget"));
		}
	}

	return nullptr;
}

void ULSSkillLoadoutEntryWidget::RefreshName(const FLSCharacterSkillRow* Row)
{
	if (NameText)
	{
		FText NameValue = FText::GetEmpty();
		if (Row && !Row->Skill_Name.IsEmpty())
		{
			NameValue = Row->Skill_Name;
		}
		else if (SkillData)
		{
			NameValue = SkillData->DisplayName;
		}
		else if (!EmptyNameText.IsEmpty())
		{
			NameValue = EmptyNameText;
		}

		if (!NamePrefixText.IsEmpty())
		{
			NameValue = NameValue.IsEmpty()
				? NamePrefixText
				: FText::Format(LOCTEXT("PrefixedSkillName", "{0} - {1}"), NamePrefixText, NameValue);
		}

		NameText->SetText(NameValue);
	}
}

void ULSSkillLoadoutEntryWidget::RefreshDescription(const FLSCharacterSkillRow* Row)
{
	if (DescriptionText)
	{
		FText DescriptionValue = FText::GetEmpty();
		if (Row && !Row->Skill_Info.IsEmpty())
		{
			DescriptionValue = Row->Skill_Info;
		}
		else if (SkillData)
		{
			DescriptionValue = SkillData->Description;
		}
		else if (!EmptyDescriptionText.IsEmpty())
		{
			DescriptionValue = EmptyDescriptionText;
		}
		DescriptionText->SetText(DescriptionValue);
	}
}

#undef LOCTEXT_NAMESPACE
