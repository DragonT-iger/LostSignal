#include "UI/Skill/LSSkillSlotWidget.h"

#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Core/LSPlayerControllerBase.h"
#include "Data/LSChipStats.h"
#include "Data/LSGameDataSubsystem.h"
#include "Data/LSProtocolTypes.h"
#include "Data/LSProtocolUnlockRow.h"
#include "LostSignal.h"
#include "Session/LSSaveSubsystem.h"
#include "Skills/LSPlayerSkillComponent.h"
#include "Skills/LSSkillDataAsset.h"

void ULSSkillSlotWidget::InitializeSlot(ULSPlayerSkillComponent* InSkillComponent, ELSPlayerSkillSlot InSlot)
{
	SkillComponent = InSkillComponent;
	Slot = InSlot;
	RefreshSkillIcon();
	RefreshCooldown();
}

void ULSSkillSlotWidget::SetPreviewBattleProtocolLevels(const int32 CurrentBattleProtocol, const int32 PreviousBattleProtocol)
{
	bUsePreviewBattleProtocolLevels = true;
	PreviewCurrentBattleProtocol = FMath::Max(CurrentBattleProtocol, 0);
	PreviewPreviousBattleProtocol = FMath::Max(PreviousBattleProtocol, 0);
	RefreshCooldown();
}

void ULSSkillSlotWidget::ClearPreviewBattleProtocolLevels()
{
	bUsePreviewBattleProtocolLevels = false;
	PreviewCurrentBattleProtocol = 0;
	PreviewPreviousBattleProtocol = 0;
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

	const bool bShowCooldownNumber = IsCooldownNumberProtocolVisible();
	const bool bShowCooldownGauge = IsCooldownGaugeProtocolVisible();
	CooldownText->SetVisibility(bShowCooldownNumber ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	CooldownBar->SetVisibility(bShowCooldownGauge ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	CooldownText->SetText(FText::AsNumber(FMath::CeilToInt(Remaining)));
	CooldownBar->SetPercent(FMath::Clamp(Remaining / Total, 0.0f, 1.0f));
}

bool ULSSkillSlotWidget::IsCooldownNumberProtocolVisible() const
{
	int32 CurrentLevel = 0;
	int32 PreviousLevel = 0;
	ResolveBattleProtocolLevels(CurrentLevel, PreviousLevel);

	UGameInstance* GameInstance = GetGameInstance();
	const ULSGameDataSubsystem* GameDataSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSGameDataSubsystem>() : nullptr;
	if (!GameDataSubsystem)
	{
		return CurrentLevel >= 2;
	}

	const FLSProtocolUnlockRow* Row = GameDataSubsystem->FindProtocolUnlockRowByEnableName(
		ELSProtocolType::Battle,
		TEXT("Skill_Cooldown"),
		TEXT("SkillSlotCooldownNumberProtocol"));
	return Row ? GameDataSubsystem->IsProtocolUnlockVisible(*Row, CurrentLevel, PreviousLevel) : CurrentLevel >= 2;
}

bool ULSSkillSlotWidget::IsCooldownGaugeProtocolVisible() const
{
	int32 CurrentLevel = 0;
	int32 PreviousLevel = 0;
	ResolveBattleProtocolLevels(CurrentLevel, PreviousLevel);

	UGameInstance* GameInstance = GetGameInstance();
	const ULSGameDataSubsystem* GameDataSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSGameDataSubsystem>() : nullptr;
	if (!GameDataSubsystem)
	{
		return CurrentLevel >= 3;
	}

	const FLSProtocolUnlockRow* Row = GameDataSubsystem->FindProtocolUnlockRowByEnableName(
		ELSProtocolType::Battle,
		TEXT("Skill_Cooldown_Gauge"),
		TEXT("SkillSlotCooldownGaugeProtocol"));
	return Row ? GameDataSubsystem->IsProtocolUnlockVisible(*Row, CurrentLevel, PreviousLevel) : CurrentLevel >= 3;
}

void ULSSkillSlotWidget::ResolveBattleProtocolLevels(int32& OutCurrentLevel, int32& OutPreviousLevel) const
{
	OutCurrentLevel = 0;
	OutPreviousLevel = 0;

	if (bUsePreviewBattleProtocolLevels)
	{
		OutCurrentLevel = PreviewCurrentBattleProtocol;
		OutPreviousLevel = PreviewPreviousBattleProtocol;
		return;
	}

	if (const ALSPlayerControllerBase* PlayerController = GetOwningPlayer<ALSPlayerControllerBase>())
	{
		if (PlayerController->HasProtocolTestLevel(ELSProtocolType::Battle))
		{
			OutCurrentLevel = PlayerController->GetProtocolTestLevel(ELSProtocolType::Battle);
			OutPreviousLevel = OutCurrentLevel;
			return;
		}
	}

	UGameInstance* GameInstance = GetGameInstance();
	const ULSSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
	if (!SaveSubsystem)
	{
		return;
	}

	const int32 InactiveSlotCount = LSChipStats::ResolveInactiveSignalSlotCount(SaveSubsystem->GetChipSignalGaugePercent());
	const TArray<FLSSessionItem> ActiveEquipmentItems = LSChipStats::BuildSignalActiveEquipmentItems(SaveSubsystem->GetChipEquipmentSlots(), InactiveSlotCount);
	OutCurrentLevel = LSChipStats::AggregateChipProtocolTotals(ActiveEquipmentItems, this).Battle;
	OutPreviousLevel = LSChipStats::AggregateChipProtocolTotals(SaveSubsystem->GetChipEquipmentSlots(), this).Battle;
}
