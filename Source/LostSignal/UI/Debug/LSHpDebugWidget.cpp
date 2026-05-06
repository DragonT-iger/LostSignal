#include "UI/Debug/LSHpDebugWidget.h"

#include "AbilitySystemComponent.h"
#include "Characters/LSCharacterBase.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"
#include "GAS/LSCombatAttributeSet.h"

#define LOCTEXT_NAMESPACE "LSHpDebugWidget"

void ULSHpDebugWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RefreshObservedCharacter();
	UpdateHealthDisplay();
}

void ULSHpDebugWidget::NativeDestruct()
{
	UnbindFromObservedASC();
	ObservedCharacter.Reset();

	Super::NativeDestruct();
}

void ULSHpDebugWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	RefreshObservedCharacter();
}

void ULSHpDebugWidget::RefreshObservedCharacter()
{
	APawn* OwnerPawn = GetOwningPlayerPawn();
	ALSCharacterBase* NewCharacter = Cast<ALSCharacterBase>(OwnerPawn);
	if (ObservedCharacter.Get() == NewCharacter)
	{
		return;
	}

	ObservedCharacter = NewCharacter;
	BindToObservedASC(NewCharacter ? NewCharacter->GetAbilitySystemComponent() : nullptr);
	UpdateHealthDisplay();
}

void ULSHpDebugWidget::BindToObservedASC(UAbilitySystemComponent* NewASC)
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
		.AddUObject(this, &ULSHpDebugWidget::HandleCurrentHealthChanged);

	MaxHealthChangedHandle = NewASC->GetGameplayAttributeValueChangeDelegate(ULSCombatAttributeSet::GetMaxHealthAttribute())
		.AddUObject(this, &ULSHpDebugWidget::HandleMaxHealthChanged);
}

void ULSHpDebugWidget::UnbindFromObservedASC()
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
	}

	CurrentHealthChangedHandle.Reset();
	MaxHealthChangedHandle.Reset();
	ObservedASC.Reset();
}

void ULSHpDebugWidget::UpdateHealthDisplay()
{
	const ULSCombatAttributeSet* CombatAttributeSet = ResolveCombatAttributeSet();
	const float CurrentHealth = CombatAttributeSet ? CombatAttributeSet->GetCurrentHealth() : 0.0f;
	const float MaxHealth = CombatAttributeSet ? CombatAttributeSet->GetMaxHealth() : 0.0f;
	const float HealthPercent = MaxHealth > 0.0f ? (CurrentHealth / MaxHealth) : 0.0f;

	if (HealthProgressBar)
	{
		HealthProgressBar->SetPercent(HealthPercent);
	}

	if (HealthText)
	{
		HealthText->SetText(FText::Format(
			LOCTEXT("HealthFormat", "HP {0} / {1}"),
			FText::AsNumber(FMath::RoundToInt(CurrentHealth)),
			FText::AsNumber(FMath::RoundToInt(MaxHealth))));
	}

	if (CharacterNameText)
	{
		CharacterNameText->SetText(
			ObservedCharacter.IsValid()
				? FText::FromString(GetNameSafe(ObservedCharacter.Get()))
				: LOCTEXT("NoCharacter", "No Character"));
	}
}

void ULSHpDebugWidget::HandleCurrentHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	UpdateHealthDisplay();
}

void ULSHpDebugWidget::HandleMaxHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	UpdateHealthDisplay();
}

const ULSCombatAttributeSet* ULSHpDebugWidget::ResolveCombatAttributeSet() const
{
	return ObservedCharacter.IsValid() ? ObservedCharacter->GetCombatAttributeSet() : nullptr;
}

#undef LOCTEXT_NAMESPACE
