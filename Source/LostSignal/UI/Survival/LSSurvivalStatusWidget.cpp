#include "UI/Survival/LSSurvivalStatusWidget.h"

#include "AbilitySystemComponent.h"
#include "Characters/LSCharacterBase.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Core/LSPlayerControllerBase.h"
#include "Data/LSChipStats.h"
#include "Data/LSGameDataSubsystem.h"
#include "Data/LSProtocolUnlockRow.h"
#include "GAS/LSCharacterAttributeSet.h"
#include "GAS/LSCombatAttributeSet.h"
#include "LostSignal.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Session/LSSaveSubsystem.h"
#include "Session/LSSessionSubsystem.h"

#define LOCTEXT_NAMESPACE "LSSurvivalStatusWidget"

namespace
{
const FName ProgressParameterName(TEXT("Progress"));
constexpr int32 HealthProgressFillProtocolLevel = 2;
constexpr int32 StaminaProgressFillProtocolLevel = 2;

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
	if (!StaminaProgressBar)
	{
		UE_LOG(LogLS, Warning, TEXT("%s is missing required survival widget binding: StaminaProgressBar."), *GetNameSafe(this));
	}
	if (!SurvivalCooldownRingImage)
	{
		UE_LOG(LogLS, Warning, TEXT("%s is missing required survival widget binding: SurvivalCooldownRingImage."), *GetNameSafe(this));
	}

	if (SurvivalCooldownRingImage)
	{
		SurvivalCooldownRingMaterial = SurvivalCooldownRingImage->GetDynamicMaterial();
		if (!SurvivalCooldownRingMaterial)
		{
			UE_LOG(LogLS, Warning, TEXT("%s cannot create survival cooldown ring material. Check SurvivalCooldownRingImage brush material."), *GetNameSafe(this));
		}
	}

	if (bStartPreviewRingCooldownOnConstruct)
	{
		StartPreviewRingCooldown(PreviewRingCooldownDuration);
	}
	else
	{
		SetRingCooldownProgress(0.0f);
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
	PreviewRingCooldownRemaining = 0.0f;
	SetRingCooldownProgress(1.0f);

	RefreshDisplay();
}

void ULSSurvivalStatusWidget::StartPreviewRingCooldown(float Duration)
{
	PreviewRingCooldownDuration = FMath::Max(Duration, 0.0f);
	PreviewRingCooldownRemaining = PreviewRingCooldownDuration;
	SetRingCooldownProgress(PreviewRingCooldownDuration > 0.0f ? 1.0f : 0.0f);
}

void ULSSurvivalStatusWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	RefreshPreviewRingCooldown(InDeltaTime);
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
	SetWidgetVisibility(StaminaProgressBar, bShowStaminaBar);
}

void ULSSurvivalStatusWidget::RefreshPreviewRingCooldown(float InDeltaTime)
{
	if (PreviewRingCooldownRemaining <= 0.0f || PreviewRingCooldownDuration <= 0.0f)
	{
		return;
	}

	PreviewRingCooldownRemaining = FMath::Max(PreviewRingCooldownRemaining - InDeltaTime, 0.0f);
	SetRingCooldownProgress(PreviewRingCooldownRemaining / PreviewRingCooldownDuration);
}

void ULSSurvivalStatusWidget::SetRingCooldownProgress(float Progress)
{
	if (SurvivalCooldownRingImage)
	{
		SurvivalCooldownRingImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	if (SurvivalCooldownRingMaterial)
	{
		SurvivalCooldownRingMaterial->SetScalarParameterValue(ProgressParameterName, FMath::Clamp(Progress, 0.0f, 1.0f));
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
