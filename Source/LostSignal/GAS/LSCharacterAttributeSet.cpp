#include "GAS/LSCharacterAttributeSet.h"

#include "GameplayEffectExtension.h"

void ULSCharacterAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
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

void ULSCharacterAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetCurrentHealthAttribute())
	{
		ClampCurrentHealth();
	}
	else if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
	{
		SetMaxHealth(FMath::Max(0.0f, GetMaxHealth()));
		ClampCurrentHealth();
	}
}

void ULSCharacterAttributeSet::ClampCurrentHealth()
{
	SetCurrentHealth(FMath::Clamp(GetCurrentHealth(), 0.0f, GetMaxHealth()));
}
