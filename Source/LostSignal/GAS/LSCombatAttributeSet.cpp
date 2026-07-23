#include "GAS/LSCombatAttributeSet.h"

#include "GameplayEffectExtension.h"
#include "GAS/LSGameplayTags.h"
#include "Net/UnrealNetwork.h"

void ULSCombatAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(ULSCombatAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ULSCombatAttributeSet, CurrentHealth, COND_None, REPNOTIFY_Always);
}

void ULSCombatAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetCurrentHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, GetMinimumHealth(), GetMaxHealth());
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(0.0f, NewValue);
	}
}

void ULSCombatAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetCurrentHealthAttribute())
	{
		ClampCurrentHealth();
	}
	else if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		const float LocalDamage = FMath::Max(0.0f, GetDamage());
		SetDamage(0.0f);

		if (LocalDamage > 0.0f)
		{
			SetCurrentHealth(GetCurrentHealth() - LocalDamage);
			ClampCurrentHealth();
		}
	}
	else if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
	{
		SetMaxHealth(FMath::Max(0.0f, GetMaxHealth()));
		ClampCurrentHealth();
	}
}

float ULSCombatAttributeSet::GetMinimumHealth() const
{
	const UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	const bool bCannotDie = ASC && ASC->HasMatchingGameplayTag(LSGameplayTags::State_CannotDie);
	return bCannotDie ? FMath::Min(1.0f, GetMaxHealth()) : 0.0f;
}

void ULSCombatAttributeSet::ClampCurrentHealth()
{
	SetCurrentHealth(FMath::Clamp(GetCurrentHealth(), GetMinimumHealth(), GetMaxHealth()));
}

void ULSCombatAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ULSCombatAttributeSet, MaxHealth, OldMaxHealth);
}

void ULSCombatAttributeSet::OnRep_CurrentHealth(const FGameplayAttributeData& OldCurrentHealth) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ULSCombatAttributeSet, CurrentHealth, OldCurrentHealth);
}
