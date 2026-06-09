#include "UI/Survival/LSSurvivalOverheadWidget.h"

#include "AbilitySystemComponent.h"
#include "Characters/LSCharacterBase.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Core/LSPlayerControllerBase.h"
#include "Data/LSChipStats.h"
#include "Data/LSGameDataSubsystem.h"
#include "Data/LSProtocolUnlockRow.h"
#include "GAS/LSCharacterAttributeSet.h"
#include "GAS/LSCombatAttributeSet.h"
#include "LostSignal.h"
#include "Session/LSSaveSubsystem.h"
#include "Session/LSSessionSubsystem.h"

#define LOCTEXT_NAMESPACE "LSSurvivalOverheadWidget"

namespace
{
FText BuildValueText(const float CurrentValue, const float MaxValue)
{
	return FText::Format(
		LOCTEXT("SurvivalOverheadValueFormat", "{0}/{1}"),
		FText::AsNumber(FMath::RoundToInt(CurrentValue)),
		FText::AsNumber(FMath::RoundToInt(MaxValue)));
}
}

void ULSSurvivalOverheadWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!HealthProgressBar)
	{
		UE_LOG(LogLS, Warning, TEXT("%s is missing required survival overhead binding: HealthProgressBar."), *GetNameSafe(this));
	}
	if (!StaminaProgressBar)
	{
		UE_LOG(LogLS, Warning, TEXT("%s is missing required survival overhead binding: StaminaProgressBar."), *GetNameSafe(this));
	}
	if (!HealthText)
	{
		UE_LOG(LogLS, Warning, TEXT("%s is missing required survival overhead binding: HealthText."), *GetNameSafe(this));
	}
	if (!StaminaText)
	{
		UE_LOG(LogLS, Warning, TEXT("%s is missing required survival overhead binding: StaminaText."), *GetNameSafe(this));
	}

	RefreshDisplay();
}

void ULSSurvivalOverheadWidget::NativeDestruct()
{
	UnbindFromObservedASC();
	ObservedCharacter.Reset();

	Super::NativeDestruct();
}

void ULSSurvivalOverheadWidget::InitializeSurvivalOverheadForCharacter(ALSCharacterBase* InCharacter)
{
	if (ObservedCharacter.Get() == InCharacter)
	{
		RefreshDisplay();
		return;
	}

	ObservedCharacter = InCharacter;
	BindToObservedASC(InCharacter ? InCharacter->GetAbilitySystemComponent() : nullptr);
	RefreshDisplay();
}

void ULSSurvivalOverheadWidget::BindToObservedASC(UAbilitySystemComponent* NewASC)
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
		.AddUObject(this, &ULSSurvivalOverheadWidget::HandleAttributeChanged);
	MaxHealthChangedHandle = NewASC->GetGameplayAttributeValueChangeDelegate(ULSCombatAttributeSet::GetMaxHealthAttribute())
		.AddUObject(this, &ULSSurvivalOverheadWidget::HandleAttributeChanged);
	CurrentStaminaChangedHandle = NewASC->GetGameplayAttributeValueChangeDelegate(ULSCharacterAttributeSet::GetCurrentStaminaAttribute())
		.AddUObject(this, &ULSSurvivalOverheadWidget::HandleAttributeChanged);
	MaxStaminaChangedHandle = NewASC->GetGameplayAttributeValueChangeDelegate(ULSCharacterAttributeSet::GetMaxStaminaAttribute())
		.AddUObject(this, &ULSSurvivalOverheadWidget::HandleAttributeChanged);
}

void ULSSurvivalOverheadWidget::UnbindFromObservedASC()
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

void ULSSurvivalOverheadWidget::RefreshDisplay()
{
	const ULSCombatAttributeSet* CombatAttributeSet = ObservedCharacter.IsValid() ? ObservedCharacter->GetCombatAttributeSet() : nullptr;
	const ULSCharacterAttributeSet* CharacterAttributeSet = ObservedASC.IsValid() ? ObservedASC->GetSet<ULSCharacterAttributeSet>() : nullptr;

	const float CurrentHealth = CombatAttributeSet ? CombatAttributeSet->GetCurrentHealth() : 0.0f;
	const float MaxHealth = CombatAttributeSet ? CombatAttributeSet->GetMaxHealth() : 0.0f;
	const float CurrentStamina = CharacterAttributeSet ? CharacterAttributeSet->GetCurrentStamina() : 0.0f;
	const float MaxStamina = CharacterAttributeSet ? CharacterAttributeSet->GetMaxStamina() : 0.0f;

	if (HealthProgressBar)
	{
		HealthProgressBar->SetPercent(MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f);
	}
	if (StaminaProgressBar)
	{
		StaminaProgressBar->SetPercent(MaxStamina > 0.0f ? CurrentStamina / MaxStamina : 0.0f);
	}
	if (HealthText)
	{
		HealthText->SetText(BuildValueText(CurrentHealth, MaxHealth));
	}
	if (StaminaText)
	{
		StaminaText->SetText(BuildValueText(CurrentStamina, MaxStamina));
	}

	RefreshVisibility();
}

void ULSSurvivalOverheadWidget::RefreshVisibility()
{
	const bool bShowHealth = IsSurvivalFeatureVisible(TEXT("HP_Overhead"));
	const bool bShowStamina = IsSurvivalFeatureVisible(TEXT("Stamina_Overhead"));
	SetWidgetVisibility(HealthProgressBar, bShowHealth);
	SetWidgetVisibility(HealthText, bShowHealth);
	SetWidgetVisibility(StaminaProgressBar, bShowStamina);
	SetWidgetVisibility(StaminaText, bShowStamina);
	SetVisibility((bShowHealth || bShowStamina) ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}

bool ULSSurvivalOverheadWidget::IsSurvivalFeatureVisible(const FName EnableName) const
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

	const FLSProtocolUnlockRow* Row = GameDataSubsystem->FindProtocolUnlockRowByEnableName(ELSProtocolType::Survival, EnableName, TEXT("SurvivalOverhead"));
	return Row ? GameDataSubsystem->IsProtocolUnlockVisible(*Row, CurrentLevel, PreviousLevel) : true;
}

void ULSSurvivalOverheadWidget::ResolveSurvivalProtocolLevels(int32& OutCurrentLevel, int32& OutPreviousLevel) const
{
	OutCurrentLevel = 0;
	OutPreviousLevel = 0;

	if (const ALSPlayerControllerBase* PlayerController = GetOwningPlayer<ALSPlayerControllerBase>())
	{
		if (PlayerController->HasProtocolTestLevel(ELSProtocolType::Survival))
		{
			OutCurrentLevel = PlayerController->GetProtocolTestLevel(ELSProtocolType::Survival);
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

void ULSSurvivalOverheadWidget::SetWidgetVisibility(UWidget* Widget, const bool bVisible) const
{
	if (Widget)
	{
		Widget->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void ULSSurvivalOverheadWidget::HandleAttributeChanged(const FOnAttributeChangeData& ChangeData)
{
	RefreshDisplay();
}

#undef LOCTEXT_NAMESPACE
