#include "UI/Skill/LSSkillLoadoutEntryWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Data/LSCharacterSkillRow.h"
#include "Data/LSGameDataSubsystem.h"
#include "Engine/GameInstance.h"
#include "LostSignal.h"
#include "Skills/LSSkillDataAsset.h"

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
}

void ULSSkillLoadoutEntryWidget::SetEquipped(const bool bInEquipped)
{
	bEquipped = bInEquipped;
	if (SelectButton)
	{
		// 이미 장착된 후보는 다시 클릭하지 못하게 막는다.
		SelectButton->SetIsEnabled(!bEquipped);
	}
}

void ULSSkillLoadoutEntryWidget::HandleSelectButtonClicked()
{
	if (!SkillData)
	{
		return;
	}

	OnEntryClicked.ExecuteIfBound(SkillData->GetSkillID());
}

void ULSSkillLoadoutEntryWidget::RefreshDisplay()
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

	// DataTable(row)이 이름/설명의 단일 출처다. 기획이 DataAsset 대신 DataTable을 채운다. 조회 실패 시 DataAsset로 폴백.
	const FLSCharacterSkillRow* Row = nullptr;
	if (SkillData)
	{
		if (const UGameInstance* GameInstance = GetGameInstance())
		{
			if (const ULSGameDataSubsystem* GameDataSubsystem = GameInstance->GetSubsystem<ULSGameDataSubsystem>())
			{
				Row = GameDataSubsystem->FindActiveSkillRowByID(SkillData->GetSkillID(), TEXT("LSSkillLoadoutEntryWidget"));
			}
		}
	}

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
		NameText->SetText(NameValue);
	}

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
		DescriptionText->SetText(DescriptionValue);
	}
}
