#include "UI/LSPlayerHUDWidget.h"

#include "Components/CanvasPanelSlot.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Core/LSPlayerControllerBase.h"
#include "Data/LSChipStats.h"
#include "Data/LSGameDataSubsystem.h"
#include "Data/LSProtocolUnlockRow.h"
#include "Data/LSProtocolTypes.h"
#include "LostSignal.h"
#include "Session/LSSaveSubsystem.h"
#include "Skills/LSPlayerSkillComponent.h"
#include "UI/Combat/LSCombatBuffListWidget.h"
#include "UI/Combat/LSDamageNumberWidget.h"
#include "UI/Combat/LSSkillCastGaugeWidget.h"
#include "UI/Minimap/LSMinimapWidget.h"
#include "UI/Noise/LSSoundDirectionIndicatorWidget.h"
#include "UI/Skill/LSSkillBarWidget.h"
#include "UI/Survival/LSSurvivalStatusWidget.h"

namespace
{
constexpr int32 SoundIndicatorFallbackSurvivalProtocolLevel = 5;
}

void ULSPlayerHUDWidget::InitializeHUDForPawn(APawn* InPawn)
{
	if (!SkillBar)
	{
		UE_LOG(LogLS, Warning, TEXT("%s cannot initialize HUD because SkillBar is not bound."), *GetNameSafe(this));
	}

	if (!Minimap)
	{
		UE_LOG(LogLS, Warning, TEXT("%s cannot initialize HUD because Minimap is not bound."), *GetNameSafe(this));
	}
	else
	{
		Minimap->InitializeMinimapForPawn(InPawn);
	}

	if (!SurvivalStatus)
	{
		UE_LOG(LogLS, Warning, TEXT("%s cannot initialize HUD because SurvivalStatus is not bound."), *GetNameSafe(this));
	}
	else
	{
		SurvivalStatus->InitializeSurvivalStatusForPawn(InPawn);
	}

	if (!SoundIndicator)
	{
		UE_LOG(LogLS, Warning, TEXT("%s cannot initialize HUD because SoundIndicator is not bound."), *GetNameSafe(this));
	}
	else
	{
		InitializeSoundIndicatorPool(InPawn);
		if (!IsSoundIndicatorProtocolVisible())
		{
			HideSoundIndicatorPool();
		}
	}

	if (!CombatBuffList)
	{
		UE_LOG(LogLS, Warning, TEXT("%s cannot initialize HUD because CombatBuffList is not bound."), *GetNameSafe(this));
	}
	else
	{
		CombatBuffList->InitializeBuffListForPawn(InPawn);
	}

	ULSPlayerSkillComponent* SkillComponent = InPawn ? InPawn->FindComponentByClass<ULSPlayerSkillComponent>() : nullptr;
	if (!SkillComponent)
	{
		UE_LOG(LogLS, Warning, TEXT("%s cannot initialize skill bar because pawn skill component is missing. Pawn=%s"),
			*GetNameSafe(this),
			*GetNameSafe(InPawn));
		return;
	}

	if (SkillBar)
	{
		SkillBar->InitializeSkillBar(SkillComponent);
	}
}

void ULSPlayerHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!SkillBar)
	{
		UE_LOG(LogLS, Warning, TEXT("%s is missing required HUD widget binding: SkillBar."), *GetNameSafe(this));
	}
	if (!Minimap)
	{
		UE_LOG(LogLS, Warning, TEXT("%s is missing required HUD widget binding: Minimap."), *GetNameSafe(this));
	}
	if (!SurvivalStatus)
	{
		UE_LOG(LogLS, Warning, TEXT("%s is missing required HUD widget binding: SurvivalStatus."), *GetNameSafe(this));
	}
	if (!SoundIndicator)
	{
		UE_LOG(LogLS, Warning, TEXT("%s is missing required HUD widget binding: SoundIndicator."), *GetNameSafe(this));
	}
	if (!CombatBuffList)
	{
		UE_LOG(LogLS, Warning, TEXT("%s is missing required HUD widget binding: CombatBuffList."), *GetNameSafe(this));
	}
	if (!SkillCastGauge)
	{
		UE_LOG(LogLS, Warning, TEXT("%s is missing required HUD widget binding: SkillCastGauge."), *GetNameSafe(this));
	}
}

void ULSPlayerHUDWidget::HandleNoiseForSoundIndicator(
	const FVector NoiseLocation,
	const float RadiusCm,
	const FGameplayTag NoiseTag,
	AActor* NoiseInstigator)
{
	(void)NoiseTag;
	(void)NoiseInstigator;

	if (!SoundIndicator || RadiusCm <= 0.0f)
	{
		return;
	}

	if (!IsSoundIndicatorProtocolVisible())
	{
		HideSoundIndicatorPool();
		return;
	}

	ULSSoundDirectionIndicatorWidget* Indicator = AcquireSoundIndicator();
	if (!Indicator)
	{
		return;
	}

	Indicator->ShowSoundDirectionFromActor(NoiseInstigator, NoiseLocation, 1.0f, RadiusCm);
}

void ULSPlayerHUDWidget::ShowDamageNumber(const FLSDamageNumberPayload& Payload)
{
	if (Payload.DamageAmount <= 0.0f || !IsDamageNumberProtocolVisible())
	{
		return;
	}

	ULSDamageNumberWidget* DamageNumber = AcquireDamageNumberWidget();
	if (!DamageNumber)
	{
		return;
	}

	DamageNumber->ShowDamageNumber(Payload);
}

void ULSPlayerHUDWidget::ShowSkillCastGauge(const FText Label, const float Duration)
{
	if (!SkillCastGauge)
	{
		UE_LOG(LogLS, Warning, TEXT("%s cannot show skill cast gauge because SkillCastGauge is not bound."), *GetNameSafe(this));
		return;
	}

	SkillCastGauge->StartCastGauge(Label, Duration);
}

void ULSPlayerHUDWidget::HideSkillCastGauge()
{
	if (SkillCastGauge)
	{
		SkillCastGauge->StopCastGauge();
	}
}

void ULSPlayerHUDWidget::InitializeSoundIndicatorPool(APawn* InPawn)
{
	if (!SoundIndicator)
	{
		return;
	}

	if (!SoundIndicatorPool.IsEmpty())
	{
		for (ULSSoundDirectionIndicatorWidget* Indicator : SoundIndicatorPool)
		{
			if (Indicator)
			{
				Indicator->InitializeSoundDirectionIndicator(InPawn);
			}
		}
		return;
	}

	const int32 TargetCount = FMath::Clamp(MaxSoundIndicatorCount, 1, 5);
	SoundIndicator->InitializeSoundDirectionIndicator(InPawn);
	SoundIndicator->HideSoundDirection();
	SoundIndicatorPool.Add(SoundIndicator);

	UPanelWidget* ParentPanel = SoundIndicator->GetParent();
	if (!ParentPanel)
	{
		UE_LOG(LogLS, Warning, TEXT("%s cannot create pooled sound indicators because SoundIndicator has no parent panel."), *GetNameSafe(this));
		return;
	}

	while (SoundIndicatorPool.Num() < TargetCount)
	{
		ULSSoundDirectionIndicatorWidget* PooledIndicator = CreatePooledSoundIndicator(ParentPanel);
		if (!PooledIndicator)
		{
			break;
		}

		PooledIndicator->InitializeSoundDirectionIndicator(InPawn);
		PooledIndicator->HideSoundDirection();
		SoundIndicatorPool.Add(PooledIndicator);
	}
}

void ULSPlayerHUDWidget::HideSoundIndicatorPool()
{
	for (ULSSoundDirectionIndicatorWidget* Indicator : SoundIndicatorPool)
	{
		if (Indicator)
		{
			Indicator->HideSoundDirection();
		}
	}
}

ULSSoundDirectionIndicatorWidget* ULSPlayerHUDWidget::AcquireSoundIndicator()
{
	if (SoundIndicatorPool.IsEmpty())
	{
		InitializeSoundIndicatorPool(GetOwningPlayerPawn());
	}

	for (ULSSoundDirectionIndicatorWidget* Indicator : SoundIndicatorPool)
	{
		if (Indicator && !Indicator->IsSoundDirectionActive())
		{
			return Indicator;
		}
	}

	ULSSoundDirectionIndicatorWidget* BestIndicator = nullptr;
	float BestRemainingTime = TNumericLimits<float>::Max();
	for (ULSSoundDirectionIndicatorWidget* Indicator : SoundIndicatorPool)
	{
		if (!Indicator)
		{
			continue;
		}

		const float RemainingTime = Indicator->GetSoundDirectionRemainingTime();
		if (RemainingTime < BestRemainingTime)
		{
			BestRemainingTime = RemainingTime;
			BestIndicator = Indicator;
		}
	}

	return BestIndicator;
}

ULSSoundDirectionIndicatorWidget* ULSPlayerHUDWidget::CreatePooledSoundIndicator(UPanelWidget* ParentPanel)
{
	if (!ParentPanel || !SoundIndicator)
	{
		return nullptr;
	}

	ULSSoundDirectionIndicatorWidget* PooledIndicator = CreateWidget<ULSSoundDirectionIndicatorWidget>(
		GetOwningPlayer(),
		SoundIndicator->GetClass());
	if (!PooledIndicator)
	{
		UE_LOG(LogLS, Warning, TEXT("%s failed to create pooled sound indicator."), *GetNameSafe(this));
		return nullptr;
	}

	ParentPanel->AddChild(PooledIndicator);
	ConfigurePooledSoundIndicatorSlot(PooledIndicator);
	return PooledIndicator;
}

void ULSPlayerHUDWidget::ConfigurePooledSoundIndicatorSlot(ULSSoundDirectionIndicatorWidget* IndicatorWidget) const
{
	if (!IndicatorWidget)
	{
		return;
	}

	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(IndicatorWidget->Slot))
	{
		CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		CanvasSlot->SetOffsets(FMargin(0.0f));
		CanvasSlot->SetAlignment(FVector2D::ZeroVector);
		return;
	}

	if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(IndicatorWidget->Slot))
	{
		OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
		OverlaySlot->SetVerticalAlignment(VAlign_Fill);
	}
}

bool ULSPlayerHUDWidget::IsSoundIndicatorProtocolVisible() const
{
	if (bDebugIgnoreSoundIndicatorProtocolLevel)
	{
		return true;
	}

	UGameInstance* GameInstance = GetGameInstance();
	const ULSGameDataSubsystem* GameDataSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSGameDataSubsystem>() : nullptr;

	int32 CurrentLevel = 0;
	int32 PreviousLevel = 0;
	bool bResolvedTestLevel = false;
	if (const ALSPlayerControllerBase* PlayerController = GetOwningPlayer<ALSPlayerControllerBase>())
	{
		if (PlayerController->HasProtocolTestLevel(ELSProtocolType::Survival))
		{
			CurrentLevel = PlayerController->GetProtocolTestLevel(ELSProtocolType::Survival);
			PreviousLevel = CurrentLevel;
			bResolvedTestLevel = true;
		}
	}

	if (!bResolvedTestLevel)
	{
		const ULSSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
		if (SaveSubsystem)
		{
			const int32 InactiveSlotCount = LSChipStats::ResolveInactiveSignalSlotCount(SaveSubsystem->GetChipSignalGaugePercent());
			const TArray<FLSSessionItem> ActiveEquipmentItems =
				LSChipStats::BuildSignalActiveEquipmentItems(SaveSubsystem->GetChipEquipmentSlots(), InactiveSlotCount);
			CurrentLevel = LSChipStats::AggregateChipProtocolTotals(ActiveEquipmentItems, this).Survival;
			PreviousLevel = LSChipStats::AggregateChipProtocolTotals(SaveSubsystem->GetChipEquipmentSlots(), this).Survival;
		}
	}

	if (!GameDataSubsystem)
	{
		if (!bLoggedMissingSoundIndicatorProtocolData)
		{
			UE_LOG(LogLS, Warning, TEXT("%s cannot resolve sound indicator protocol row because GameDataSubsystem is missing. Falling back to Survival level >= %d."),
				*GetNameSafe(this),
				SoundIndicatorFallbackSurvivalProtocolLevel);
			bLoggedMissingSoundIndicatorProtocolData = true;
		}
		return CurrentLevel >= SoundIndicatorFallbackSurvivalProtocolLevel;
	}

	const FLSProtocolUnlockRow* Row = GameDataSubsystem->FindProtocolUnlockRowByEnableName(
		ELSProtocolType::Survival,
		TEXT("Monster_Sound"),
		TEXT("PlayerHUDSoundIndicator"));
	if (!Row)
	{
		if (!bLoggedMissingSoundIndicatorProtocolData)
		{
			UE_LOG(LogLS, Warning, TEXT("%s cannot resolve sound indicator protocol row because Monster_Sound unlock row is missing. Falling back to Survival level >= %d."),
				*GetNameSafe(this),
				SoundIndicatorFallbackSurvivalProtocolLevel);
			bLoggedMissingSoundIndicatorProtocolData = true;
		}
		return CurrentLevel >= SoundIndicatorFallbackSurvivalProtocolLevel;
	}

	return GameDataSubsystem->IsProtocolUnlockVisible(*Row, CurrentLevel, PreviousLevel);
}

ULSDamageNumberWidget* ULSPlayerHUDWidget::AcquireDamageNumberWidget()
{
	for (ULSDamageNumberWidget* DamageNumber : DamageNumberPool)
	{
		if (DamageNumber && !DamageNumber->IsDamageNumberActive())
		{
			return DamageNumber;
		}
	}

	if (DamageNumberPool.Num() < FMath::Clamp(MaxDamageNumberCount, 1, 40))
	{
		return CreateDamageNumberWidget();
	}

	ULSDamageNumberWidget* BestDamageNumber = nullptr;
	float BestRemainingTime = TNumericLimits<float>::Max();
	for (ULSDamageNumberWidget* DamageNumber : DamageNumberPool)
	{
		if (!DamageNumber)
		{
			continue;
		}

		const float RemainingTime = DamageNumber->GetRemainingLifeSeconds();
		if (RemainingTime < BestRemainingTime)
		{
			BestRemainingTime = RemainingTime;
			BestDamageNumber = DamageNumber;
		}
	}

	return BestDamageNumber;
}

ULSDamageNumberWidget* ULSPlayerHUDWidget::CreateDamageNumberWidget()
{
	if (!DamageNumberWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("%s cannot create damage number widget because DamageNumberWidgetClass is not set."), *GetNameSafe(this));
		return nullptr;
	}

	ULSDamageNumberWidget* DamageNumber = CreateWidget<ULSDamageNumberWidget>(GetOwningPlayer(), DamageNumberWidgetClass);
	if (!DamageNumber)
	{
		UE_LOG(LogLS, Warning, TEXT("%s failed to create damage number widget."), *GetNameSafe(this));
		return nullptr;
	}

	DamageNumber->AddToViewport();
	DamageNumberPool.Add(DamageNumber);
	return DamageNumber;
}

bool ULSPlayerHUDWidget::IsDamageNumberProtocolVisible() const
{
	UGameInstance* GameInstance = GetGameInstance();
	const ULSGameDataSubsystem* GameDataSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSGameDataSubsystem>() : nullptr;
	if (!GameDataSubsystem)
	{
		return true;
	}

	int32 CurrentLevel = 0;
	int32 PreviousLevel = 0;
	ResolveBattleProtocolLevels(CurrentLevel, PreviousLevel);

	const FLSProtocolUnlockRow* Row = GameDataSubsystem->FindProtocolUnlockRowByEnableName(
		ELSProtocolType::Battle,
		TEXT("Damage_Number"),
		TEXT("PlayerHUDDamageNumber"));
	return Row ? GameDataSubsystem->IsProtocolUnlockVisible(*Row, CurrentLevel, PreviousLevel) : true;
}

void ULSPlayerHUDWidget::ResolveBattleProtocolLevels(int32& OutCurrentLevel, int32& OutPreviousLevel) const
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
