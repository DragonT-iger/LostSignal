#include "UI/Survival/LSSurvivalStatusWidget.h"

#include "AbilitySystemComponent.h"
#include "Characters/LSCharacterBase.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Core/LSFarmingGameMode.h"
#include "Core/LSPlayerControllerBase.h"
#include "Kismet/GameplayStatics.h"
#include "Data/LSChipStats.h"
#include "Data/LSGameDataSubsystem.h"
#include "Data/LSProtocolUnlockRow.h"
#include "GAS/LSCharacterAttributeSet.h"
#include "GAS/LSCombatAttributeSet.h"
#include "LostSignal.h"
#include "Session/LSSaveSubsystem.h"
#include "Session/LSSessionSubsystem.h"

#define LOCTEXT_NAMESPACE "LSSurvivalStatusWidget"

namespace
{
constexpr int32 HealthProgressFillProtocolLevel = 2;
constexpr int32 HealthPreviewFillProtocolLevel = 3;
constexpr int32 StaminaProgressFillProtocolLevel = 2;
constexpr int32 SignalProgressBarCount = 10;

FText BuildSurvivalStatusValueText(const float CurrentValue, const float MaxValue)
{
	return FText::Format(
		LOCTEXT("SurvivalValueFormat", "{0}/{1}"),
		FText::AsNumber(FMath::RoundToInt(CurrentValue)),
		FText::AsNumber(FMath::RoundToInt(MaxValue)));
}
}

void ULSSurvivalStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!HealthText)
	{
		UE_LOG(LogLS, Warning, TEXT("%s is missing required survival widget binding: HealthText."), *GetNameSafe(this));
	}
	if (!StaminaText)
	{
		UE_LOG(LogLS, Warning, TEXT("%s is missing required survival widget binding: StaminaText."), *GetNameSafe(this));
	}
	if (!HealthProgressBar)
	{
		UE_LOG(LogLS, Warning, TEXT("%s is missing required survival widget binding: HealthProgressBar."), *GetNameSafe(this));
	}
	if (!HealthPreviewProgressBar)
	{
		UE_LOG(LogLS, Warning, TEXT("%s is missing required survival widget binding: HealthPreviewProgressBar."), *GetNameSafe(this));
	}
	if (!StaminaProgressBar)
	{
		UE_LOG(LogLS, Warning, TEXT("%s is missing required survival widget binding: StaminaProgressBar."), *GetNameSafe(this));
	}
	InitializeSignalProgressBars();

	if (bStartPreviewSignalCooldownOnConstruct)
	{
		StartPreviewSignalCooldown(PreviewSignalCooldownDuration);
	}
	else
	{
		SetSignalProgress(0.0f);
	}
	RefreshDisplay();
}

void ULSSurvivalStatusWidget::NativeDestruct()
{
	UnbindFromObservedASC();
	ObservedCharacter.Reset();

	Super::NativeDestruct();
}

void ULSSurvivalStatusWidget::InitializeSurvivalStatusForPawn(APawn* InPawn)
{
	bUsePreviewSurvivalStatus = false;
	ALSCharacterBase* NewCharacter = Cast<ALSCharacterBase>(InPawn);
	if (ObservedCharacter.Get() == NewCharacter)
	{
		RefreshDisplay();
		return;
	}

	ObservedCharacter = NewCharacter;
	BindToObservedASC(NewCharacter ? NewCharacter->GetAbilitySystemComponent() : nullptr);
	RefreshDisplay();

	if (!NewCharacter)
	{
		UE_LOG(LogLS, Warning, TEXT("%s cannot initialize survival status because pawn is not an LS character. Pawn=%s"),
			*GetNameSafe(this),
			*GetNameSafe(InPawn));
	}
}

void ULSSurvivalStatusWidget::SetPreviewSurvivalStatus(
	const int32 CurrentSurvivalProtocol,
	const int32 PreviousSurvivalProtocol,
	const float CurrentHealth,
	const float MaxHealth,
	const float CurrentStamina,
	const float MaxStamina)
{
	bUsePreviewSurvivalStatus = true;
	ObservedCharacter.Reset();
	UnbindFromObservedASC();

	PreviewCurrentSurvivalProtocol = FMath::Max(0, CurrentSurvivalProtocol);
	PreviewPreviousSurvivalProtocol = FMath::Max(PreviewCurrentSurvivalProtocol, PreviousSurvivalProtocol);
	PreviewMaxHealth = FMath::Max(0.0f, MaxHealth);
	PreviewCurrentHealth = FMath::Clamp(CurrentHealth, 0.0f, PreviewMaxHealth);
	PreviewMaxStamina = FMath::Max(0.0f, MaxStamina);
	PreviewCurrentStamina = FMath::Clamp(CurrentStamina, 0.0f, PreviewMaxStamina);
	PreviewSignalCooldownRemaining = 0.0f;
	SetSignalProgress(1.0f);

	RefreshDisplay();
}

void ULSSurvivalStatusWidget::SetHealthPreview(const float TargetHealth, const float Duration, const bool bIsRecovery)
{
	const ULSCombatAttributeSet* CombatAttributeSet = ResolveCombatAttributeSet();
	const float MaxHealth = bUsePreviewSurvivalStatus ? PreviewMaxHealth : (CombatAttributeSet ? CombatAttributeSet->GetMaxHealth() : 0.0f);
	if (MaxHealth <= 0.0f)
	{
		ClearHealthPreview();
		return;
	}

	bHasHealthPreview = true;
	HealthPreviewTarget = FMath::Clamp(TargetHealth, 0.0f, MaxHealth);
	HealthPreviewDuration = FMath::Max(0.0f, Duration);
	HealthPreviewRemaining = HealthPreviewDuration;
	bHealthPreviewIsRecovery = bIsRecovery;
	RefreshDisplay();
}

void ULSSurvivalStatusWidget::ClearHealthPreview()
{
	bHasHealthPreview = false;
	HealthPreviewTarget = 0.0f;
	HealthPreviewDuration = 0.0f;
	HealthPreviewRemaining = 0.0f;
	bHealthPreviewIsRecovery = true;
	RefreshDisplay();
}

void ULSSurvivalStatusWidget::StartPreviewSignalCooldown(float Duration)
{
	PreviewSignalCooldownDuration = FMath::Max(Duration, 0.0f);
	PreviewSignalCooldownRemaining = PreviewSignalCooldownDuration;
	SetSignalProgress(PreviewSignalCooldownDuration > 0.0f ? 1.0f : 0.0f);
}

void ULSSurvivalStatusWidget::SetPreviewSignalProgress(const float SignalProgress)
{
	PreviewSignalCooldownRemaining = 0.0f;
	SetSignalProgress(SignalProgress);
}

void ULSSurvivalStatusWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	RefreshPreviewSignalCooldown(InDeltaTime);
	if (!bUsePreviewSurvivalStatus)
	{
		RefreshSignalProgressFromSave();
	}
	if (HealthPreviewRemaining > 0.0f && HealthPreviewDuration > 0.0f)
	{
		HealthPreviewRemaining = FMath::Max(HealthPreviewRemaining - InDeltaTime, 0.0f);
		if (HealthPreviewRemaining <= 0.0f)
		{
			ClearHealthPreview();
		}
	}
}

void ULSSurvivalStatusWidget::BindToObservedASC(UAbilitySystemComponent* NewASC)
{
	if (ObservedASC.Get() == NewASC)
	{
		return;
	}

	UnbindFromObservedASC();
	ObservedASC = NewASC;
	if (!NewASC)
	{
		return;
	}

	CurrentHealthChangedHandle = NewASC->GetGameplayAttributeValueChangeDelegate(ULSCombatAttributeSet::GetCurrentHealthAttribute())
		.AddUObject(this, &ULSSurvivalStatusWidget::HandleAttributeChanged);
	MaxHealthChangedHandle = NewASC->GetGameplayAttributeValueChangeDelegate(ULSCombatAttributeSet::GetMaxHealthAttribute())
		.AddUObject(this, &ULSSurvivalStatusWidget::HandleAttributeChanged);
	CurrentStaminaChangedHandle = NewASC->GetGameplayAttributeValueChangeDelegate(ULSCharacterAttributeSet::GetCurrentStaminaAttribute())
		.AddUObject(this, &ULSSurvivalStatusWidget::HandleAttributeChanged);
	MaxStaminaChangedHandle = NewASC->GetGameplayAttributeValueChangeDelegate(ULSCharacterAttributeSet::GetMaxStaminaAttribute())
		.AddUObject(this, &ULSSurvivalStatusWidget::HandleAttributeChanged);
}

void ULSSurvivalStatusWidget::UnbindFromObservedASC()
{
	if (UAbilitySystemComponent* ASC = ObservedASC.Get())
	{
		if (CurrentHealthChangedHandle.IsValid())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(ULSCombatAttributeSet::GetCurrentHealthAttribute()).Remove(CurrentHealthChangedHandle);
		}
		if (MaxHealthChangedHandle.IsValid())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(ULSCombatAttributeSet::GetMaxHealthAttribute()).Remove(MaxHealthChangedHandle);
		}
		if (CurrentStaminaChangedHandle.IsValid())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(ULSCharacterAttributeSet::GetCurrentStaminaAttribute()).Remove(CurrentStaminaChangedHandle);
		}
		if (MaxStaminaChangedHandle.IsValid())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(ULSCharacterAttributeSet::GetMaxStaminaAttribute()).Remove(MaxStaminaChangedHandle);
		}
	}

	CurrentHealthChangedHandle.Reset();
	MaxHealthChangedHandle.Reset();
	CurrentStaminaChangedHandle.Reset();
	MaxStaminaChangedHandle.Reset();
	ObservedASC.Reset();
}

void ULSSurvivalStatusWidget::RefreshDisplay()
{
	const ULSCombatAttributeSet* CombatAttributeSet = ResolveCombatAttributeSet();
	const ULSCharacterAttributeSet* CharacterAttributeSet = ResolveCharacterAttributeSet();

	const float CurrentHealth = bUsePreviewSurvivalStatus ? PreviewCurrentHealth : (CombatAttributeSet ? CombatAttributeSet->GetCurrentHealth() : 0.0f);
	const float MaxHealth = bUsePreviewSurvivalStatus ? PreviewMaxHealth : (CombatAttributeSet ? CombatAttributeSet->GetMaxHealth() : 0.0f);
	const float CurrentStamina = bUsePreviewSurvivalStatus ? PreviewCurrentStamina : (CharacterAttributeSet ? CharacterAttributeSet->GetCurrentStamina() : 0.0f);
	const float MaxStamina = bUsePreviewSurvivalStatus ? PreviewMaxStamina : (CharacterAttributeSet ? CharacterAttributeSet->GetMaxStamina() : 0.0f);
	int32 CurrentSurvivalProtocolLevel = 0;
	int32 PreviousSurvivalProtocolLevel = 0;
	ResolveSurvivalProtocolLevels(CurrentSurvivalProtocolLevel, PreviousSurvivalProtocolLevel);

	if (HealthText)
	{
		HealthText->SetText(BuildSurvivalStatusValueText(CurrentHealth, MaxHealth));
	}
	if (StaminaText)
	{
		StaminaText->SetText(BuildSurvivalStatusValueText(CurrentStamina, MaxStamina));
	}
	if (HealthProgressBar)
	{
		const bool bUpdateHealthProgress = CurrentSurvivalProtocolLevel >= HealthProgressFillProtocolLevel;
		HealthProgressBar->SetPercent(bUpdateHealthProgress && MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f);
	}
	if (HealthPreviewProgressBar)
	{
		const bool bUpdateHealthProgress = CurrentSurvivalProtocolLevel >= HealthProgressFillProtocolLevel;
		const bool bUpdateHealthPreview = CurrentSurvivalProtocolLevel >= HealthPreviewFillProtocolLevel;
		const float PreviewPercent = MaxHealth > 0.0f ? ((bHasHealthPreview ? HealthPreviewTarget : CurrentHealth) / MaxHealth) : 0.0f;
		HealthPreviewProgressBar->SetPercent(bUpdateHealthProgress && bUpdateHealthPreview && MaxHealth > 0.0f ? PreviewPercent : 0.0f);
	}
	if (StaminaProgressBar)
	{
		const bool bUpdateStaminaProgress = CurrentSurvivalProtocolLevel >= StaminaProgressFillProtocolLevel;
		StaminaProgressBar->SetPercent(bUpdateStaminaProgress && MaxStamina > 0.0f ? CurrentStamina / MaxStamina : 0.0f);
	}
	RefreshVisibility();
}

void ULSSurvivalStatusWidget::RefreshVisibility()
{
	int32 CurrentSurvivalProtocolLevel = 0;
	int32 PreviousSurvivalProtocolLevel = 0;
	ResolveSurvivalProtocolLevels(CurrentSurvivalProtocolLevel, PreviousSurvivalProtocolLevel);

	const bool bShowHealthText = IsSurvivalFeatureVisible(TEXT("HP_Text"));
	const bool bShowStaminaText = IsSurvivalFeatureVisible(TEXT("Stamina_Text"));
	const bool bShowHealthBar = IsSurvivalFeatureVisible(TEXT("HP_Bar")) || CurrentSurvivalProtocolLevel >= 1;
	const bool bShowStaminaBar = IsSurvivalFeatureVisible(TEXT("Stamina_Bar")) || CurrentSurvivalProtocolLevel >= 1;

	SetWidgetVisibility(HealthText, bShowHealthText);
	SetWidgetVisibility(StaminaText, bShowStaminaText);
	SetWidgetVisibility(HealthProgressBar, bShowHealthBar);
	SetWidgetVisibility(HealthPreviewProgressBar, bShowHealthBar);
	SetWidgetVisibility(StaminaProgressBar, bShowStaminaBar);
}

void ULSSurvivalStatusWidget::InitializeSignalProgressBars()
{
	SignalProgressBars = {
		SignalProgressBar1,
		SignalProgressBar2,
		SignalProgressBar3,
		SignalProgressBar4,
		SignalProgressBar5,
		SignalProgressBar6,
		SignalProgressBar7,
		SignalProgressBar8,
		SignalProgressBar9,
		SignalProgressBar10
	};

	for (int32 Index = 0; Index < SignalProgressBars.Num(); ++Index)
	{
		if (!SignalProgressBars[Index])
		{
			UE_LOG(LogLS, Warning, TEXT("%s is missing required survival widget binding: SignalProgressBar%d."),
				*GetNameSafe(this),
				Index + 1);
		}
	}
}

void ULSSurvivalStatusWidget::RefreshPreviewSignalCooldown(float InDeltaTime)
{
	if (PreviewSignalCooldownRemaining <= 0.0f || PreviewSignalCooldownDuration <= 0.0f)
	{
		return;
	}

	PreviewSignalCooldownRemaining = FMath::Max(PreviewSignalCooldownRemaining - InDeltaTime, 0.0f);
	SetSignalProgress(PreviewSignalCooldownRemaining / PreviewSignalCooldownDuration);
}

void ULSSurvivalStatusWidget::RefreshSignalProgressFromSave()
{
	UGameInstance* GameInstance = GetGameInstance();
	const ULSSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
	if (!SaveSubsystem)
	{
		SetSignalProgress(0.0f);
		return;
	}

	const float SignalPercent = SaveSubsystem->GetChipSignalGaugePercent();

	// 신호 게이지는 드레인 타이머가 도는 곳(= ALSFarmingGameMode 가 있는 레이드/테스트 레벨)에서만
	// 시간에 따라 감소한다. 로비/결과 등 다른 게임모드에서는 캐스트가 실패하고, 타이머가 멈춰 있으면
	// 잔여시간이 음수라 아래에서 정적 표시로 폴백한다. (정식 레이드 세션 플래그가 아니라 실제 타이머 기준)
	const ALSFarmingGameMode* FarmingGameMode = Cast<ALSFarmingGameMode>(UGameplayStatics::GetGameMode(this));
	const float DrainInterval = FarmingGameMode ? FarmingGameMode->GetSignalGaugeDrainInterval() : 0.0f;
	const float DrainRemaining = FarmingGameMode ? FarmingGameMode->GetSignalGaugeDrainRemainingSeconds() : -1.0f;

	// 레이드가 아니거나(로비/프리뷰), 드레인 타이머에 접근할 수 없거나(비권한 원격 클라),
	// 타이머가 멈춰 있으면(게이지 0%) 실제 잔여시간이 없으므로 구간 위치를 정적으로 표시한다.
	if (DrainRemaining < 0.0f || DrainInterval <= 0.0f)
	{
		SetSignalProgress(SignalPercent);
		return;
	}

	// 레이드: 게이지는 1분마다 10% 단계로만 떨어지므로, 10칸 바는 자체 추정 대신 게임모드 드레인 타이머의
	// 실제 잔여시간을 현재 10% 구간에 보간해 매 틱 반영한다.
	const float StepRemaining = FMath::Clamp(DrainRemaining / DrainInterval, 0.0f, 1.0f);
	const float NextSignalPercent = FMath::Max(SignalPercent - 0.1f, 0.0f);
	PreviewSignalCooldownRemaining = 0.0f;
	SetSignalProgress(NextSignalPercent + (0.1f * StepRemaining));
}

void ULSSurvivalStatusWidget::SetSignalProgress(float Progress)
{
	const bool bShowSignalProgress = ShouldShowSignalIndicator();
	const float ScaledProgress = FMath::Clamp(Progress, 0.0f, 1.0f) * SignalProgressBarCount;

	for (int32 Index = 0; Index < SignalProgressBars.Num(); ++Index)
	{
		if (UProgressBar* ProgressBar = SignalProgressBars[Index])
		{
			ProgressBar->SetPercent(FMath::Clamp(ScaledProgress - Index, 0.0f, 1.0f));
			ProgressBar->SetVisibility(bShowSignalProgress ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		}
	}
}

void ULSSurvivalStatusWidget::ResolveSurvivalProtocolLevels(int32& OutCurrentLevel, int32& OutPreviousLevel) const
{
	OutCurrentLevel = 0;
	OutPreviousLevel = 0;

	if (bUsePreviewSurvivalStatus)
	{
		OutCurrentLevel = PreviewCurrentSurvivalProtocol;
		OutPreviousLevel = PreviewPreviousSurvivalProtocol;
		return;
	}

	if (const ALSPlayerControllerBase* PlayerController = GetOwningPlayer<ALSPlayerControllerBase>())
	{
		if (PlayerController->HasSurvivalProtocolTestLevel())
		{
			OutCurrentLevel = PlayerController->GetSurvivalProtocolTestLevel();
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
	OutCurrentLevel = LSChipStats::AggregateChipProtocolTotals(ActiveEquipmentItems, this).Survival;
	OutPreviousLevel = LSChipStats::AggregateChipProtocolTotals(SaveSubsystem->GetChipEquipmentSlots(), this).Survival;
}

bool ULSSurvivalStatusWidget::IsSurvivalFeatureVisible(const FName EnableName) const
{
	UGameInstance* GameInstance = GetGameInstance();
	const ULSGameDataSubsystem* GameDataSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSGameDataSubsystem>() : nullptr;
	if (!GameDataSubsystem)
	{
		return true;
	}

	int32 CurrentLevel = 0;
	int32 PreviousLevel = 0;
	ResolveSurvivalProtocolLevels(CurrentLevel, PreviousLevel);

	const FLSProtocolUnlockRow* Row = GameDataSubsystem->FindProtocolUnlockRowByEnableName(ELSProtocolType::Survival, EnableName, TEXT("SurvivalStatus"));
	return Row ? GameDataSubsystem->IsProtocolUnlockVisible(*Row, CurrentLevel, PreviousLevel) : true;
}

bool ULSSurvivalStatusWidget::ShouldShowSignalIndicator() const
{
	int32 CurrentSurvivalProtocolLevel = 0;
	int32 PreviousSurvivalProtocolLevel = 0;
	ResolveSurvivalProtocolLevels(CurrentSurvivalProtocolLevel, PreviousSurvivalProtocolLevel);
	return CurrentSurvivalProtocolLevel >= 1;
}

void ULSSurvivalStatusWidget::SetWidgetVisibility(UWidget* Widget, const bool bVisible) const
{
	if (Widget)
	{
		Widget->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void ULSSurvivalStatusWidget::HandleAttributeChanged(const FOnAttributeChangeData& ChangeData)
{
	RefreshDisplay();
}

const ULSCombatAttributeSet* ULSSurvivalStatusWidget::ResolveCombatAttributeSet() const
{
	return ObservedCharacter.IsValid() ? ObservedCharacter->GetCombatAttributeSet() : nullptr;
}

const ULSCharacterAttributeSet* ULSSurvivalStatusWidget::ResolveCharacterAttributeSet() const
{
	const UAbilitySystemComponent* ASC = ObservedASC.Get();
	return ASC ? ASC->GetSet<ULSCharacterAttributeSet>() : nullptr;
}

#undef LOCTEXT_NAMESPACE
