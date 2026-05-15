#include "UI/Skill/LSSkillSlotWidget.h"

#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "LostSignal.h"
#include "Skills/LSPlayerSkillComponent.h"
#include "Skills/LSSkillDataAsset.h"

void ULSSkillSlotWidget::InitializeSlot(ULSPlayerSkillComponent* InSkillComponent, ELSPlayerSkillSlot InSlot)
{
	SkillComponent = InSkillComponent;
	Slot = InSlot;
	RefreshSkillIcon();
	RefreshCooldown();
}

void ULSSkillSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!IconImage || !CooldownText || !CooldownBar)
	{
		UE_LOG(LogLS, Warning, TEXT("%s is missing required skill slot widget binding. Icon=%s CooldownText=%s CooldownBar=%s"),
			*GetNameSafe(this),
			*GetNameSafe(IconImage),
			*GetNameSafe(CooldownText),
			*GetNameSafe(CooldownBar));
		return;
	}

	CooldownText->SetVisibility(ESlateVisibility::Collapsed);
	CooldownBar->SetVisibility(ESlateVisibility::Collapsed);
}

void ULSSkillSlotWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	ULSSkillDataAsset* CurrentSkillData = SkillComponent ? SkillComponent->GetSkillData(Slot) : nullptr;
	if (CurrentSkillData != CachedSkillData)
	{
		RefreshSkillIcon();
	}

	RefreshCooldown();
}

void ULSSkillSlotWidget::RefreshSkillIcon()
{
	if (!IconImage)
	{
		return;
	}

	ULSSkillDataAsset* SkillData = SkillComponent ? SkillComponent->GetSkillData(Slot) : nullptr;
	CachedSkillData = SkillData;
	if (SkillData && SkillData->Icon)
	{
		IconImage->SetBrushFromTexture(SkillData->Icon);
	}

	IconImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void ULSSkillSlotWidget::RefreshCooldown()
{
	if (!CooldownText || !CooldownBar)
	{
		return;
	}

	ULSSkillDataAsset* SkillData = SkillComponent ? SkillComponent->GetSkillData(Slot) : nullptr;
	const float Remaining = SkillComponent ? SkillComponent->GetSkillCooldownRemaining(SkillData) : 0.0f;
	const float Total = SkillData ? SkillData->GetCooldownDuration() : 0.0f;
	if (!SkillData || Remaining <= 0.0f || Total <= 0.0f)
	{
		CooldownText->SetVisibility(ESlateVisibility::Collapsed);
		CooldownBar->SetVisibility(ESlateVisibility::Collapsed);
		CooldownBar->SetPercent(0.0f);
		return;
	}

	CooldownText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	CooldownBar->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	CooldownText->SetText(FText::AsNumber(FMath::CeilToInt(Remaining)));
	CooldownBar->SetPercent(FMath::Clamp(Remaining / Total, 0.0f, 1.0f));
}
