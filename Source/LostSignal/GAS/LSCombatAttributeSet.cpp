#include "GAS/LSCombatAttributeSet.h"

#include "GameplayEffectExtension.h"

void ULSCombatAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetCurrentHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
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

void ULSCombatAttributeSet::ClampCurrentHealth()
{
	SetCurrentHealth(FMath::Clamp(GetCurrentHealth(), 0.0f, GetMaxHealth()));
}
