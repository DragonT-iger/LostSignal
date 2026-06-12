#include "UI/Combat/LSSkillCastGaugeWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Core/LSPlayerControllerBase.h"
#include "Data/LSChipStats.h"
#include "Data/LSGameDataSubsystem.h"
#include "Data/LSProtocolTypes.h"
#include "Data/LSProtocolUnlockRow.h"
#include "LostSignal.h"
#include "Session/LSSaveSubsystem.h"

void ULSSkillCastGaugeWidget::StartCastGauge(const FText Label, const float Duration)
{
	if (Duration <= 0.0f)
	{
		StopCastGauge();
		return;
	}

	ActiveLabel = Label;
	CastDuration = Duration;
	CastStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	bCasting = true;
	RefreshCastGauge();
}

void ULSSkillCastGaugeWidget::StopCastGauge()
{
	bCasting = false;
	CastDuration = 0.0f;
	CastStartTime = 0.0f;
	SetVisibility(ESlateVisibility::Collapsed);
}

void ULSSkillCastGaugeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!CastLabelText || !CastTimeText || !CastProgressBar)
	{
		UE_LOG(LogLS, Warning, TEXT("%s is missing required skill cast gauge binding. Label=%s Time=%s Bar=%s"),
			*GetNameSafe(this),
			*GetNameSafe(CastLabelText),
			*GetNameSafe(CastTimeText),
			*GetNameSafe(CastProgressBar));
	}

	SetVisibility(ESlateVisibility::Collapsed);
}

void ULSSkillCastGaugeWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshCastGauge();
}

void ULSSkillCastGaugeWidget::RefreshCastGauge()
{
	if (!bCasting || !CastLabelText || !CastTimeText || !CastProgressBar || !IsCastGaugeProtocolVisible())
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const UWorld* World = GetWorld();
	const float Now = World ? World->GetTimeSeconds() : CastStartTime;
	const float Elapsed = FMath::Max(Now - CastStartTime, 0.0f);
	const float Remaining = FMath::Max(CastDuration - Elapsed, 0.0f);
	if (Remaining <= 0.0f)
	{
		StopCastGauge();
		return;
	}

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	CastLabelText->SetText(ActiveLabel);
	CastTimeText->SetText(FText::AsNumber(FMath::CeilToInt(Remaining)));
	CastProgressBar->SetPercent(FMath::Clamp(Elapsed / CastDuration, 0.0f, 1.0f));
}

bool ULSSkillCastGaugeWidget::IsCastGaugeProtocolVisible() const
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
		TEXT("Skill_Casting_Gauge"),
		TEXT("SkillCastGaugeProtocol"));
	return Row ? GameDataSubsystem->IsProtocolUnlockVisible(*Row, CurrentLevel, PreviousLevel) : CurrentLevel >= 3;
}

void ULSSkillCastGaugeWidget::ResolveBattleProtocolLevels(int32& OutCurrentLevel, int32& OutPreviousLevel) const
{
	OutCurrentLevel = 0;
	OutPreviousLevel = 0;

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
