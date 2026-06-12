#include "UI/Skill/LSSkillBarWidget.h"

#include "Core/LSPlayerControllerBase.h"
#include "Data/LSChipStats.h"
#include "Data/LSGameDataSubsystem.h"
#include "Data/LSProtocolUnlockRow.h"
#include "LostSignal.h"
#include "Session/LSSaveSubsystem.h"
#include "Skills/LSSkillTypes.h"
#include "UI/Skill/LSSkillSlotWidget.h"

void ULSSkillBarWidget::InitializeSkillBar(ULSPlayerSkillComponent* InSkillComponent)
{
	SkillComponent = InSkillComponent;
	RefreshProtocolVisibility();

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

void ULSSkillBarWidget::SetPreviewBattleProtocolLevels(const int32 CurrentBattleProtocol, const int32 PreviousBattleProtocol)
{
	bUsePreviewBattleProtocolLevels = true;
	PreviewCurrentBattleProtocol = FMath::Max(CurrentBattleProtocol, 0);
	PreviewPreviousBattleProtocol = FMath::Max(PreviousBattleProtocol, 0);
	if (Skill1Slot) { Skill1Slot->SetPreviewBattleProtocolLevels(PreviewCurrentBattleProtocol, PreviewPreviousBattleProtocol); }
	if (Skill2Slot) { Skill2Slot->SetPreviewBattleProtocolLevels(PreviewCurrentBattleProtocol, PreviewPreviousBattleProtocol); }
	if (Skill3Slot) { Skill3Slot->SetPreviewBattleProtocolLevels(PreviewCurrentBattleProtocol, PreviewPreviousBattleProtocol); }
	if (Skill4Slot) { Skill4Slot->SetPreviewBattleProtocolLevels(PreviewCurrentBattleProtocol, PreviewPreviousBattleProtocol); }
	if (UltimateSlot) { UltimateSlot->SetPreviewBattleProtocolLevels(PreviewCurrentBattleProtocol, PreviewPreviousBattleProtocol); }
	RefreshProtocolVisibility();
}

void ULSSkillBarWidget::ClearPreviewBattleProtocolLevels()
{
	bUsePreviewBattleProtocolLevels = false;
	PreviewCurrentBattleProtocol = 0;
	PreviewPreviousBattleProtocol = 0;
	if (Skill1Slot) { Skill1Slot->ClearPreviewBattleProtocolLevels(); }
	if (Skill2Slot) { Skill2Slot->ClearPreviewBattleProtocolLevels(); }
	if (Skill3Slot) { Skill3Slot->ClearPreviewBattleProtocolLevels(); }
	if (Skill4Slot) { Skill4Slot->ClearPreviewBattleProtocolLevels(); }
	if (UltimateSlot) { UltimateSlot->ClearPreviewBattleProtocolLevels(); }
	RefreshProtocolVisibility();
}

void ULSSkillBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshProtocolVisibility();

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

void ULSSkillBarWidget::RefreshProtocolVisibility()
{
	SetVisibility(IsSkillSlotProtocolVisible() ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}

bool ULSSkillBarWidget::IsSkillSlotProtocolVisible() const
{
	int32 CurrentLevel = 0;
	int32 PreviousLevel = 0;
	ResolveBattleProtocolLevels(CurrentLevel, PreviousLevel);

	UGameInstance* GameInstance = GetGameInstance();
	const ULSGameDataSubsystem* GameDataSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSGameDataSubsystem>() : nullptr;
	if (!GameDataSubsystem)
	{
		return CurrentLevel >= 1;
	}

	const FLSProtocolUnlockRow* Row = GameDataSubsystem->FindProtocolUnlockRowByEnableName(
		ELSProtocolType::Battle,
		TEXT("Skill_Slot"),
		TEXT("SkillBarProtocol"));
	return Row ? GameDataSubsystem->IsProtocolUnlockVisible(*Row, CurrentLevel, PreviousLevel) : CurrentLevel >= 1;
}

void ULSSkillBarWidget::ResolveBattleProtocolLevels(int32& OutCurrentLevel, int32& OutPreviousLevel) const
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
