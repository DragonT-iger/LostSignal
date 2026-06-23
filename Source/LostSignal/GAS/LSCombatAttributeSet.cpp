#include "GAS/LSCombatAttributeSet.h"

#include "GameplayEffectExtension.h"
#include "LostSignal.h"
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
	// [DEBUG] 체력 100 초기화 원인 진단용. 클램프로 현재 체력이 줄어드는 순간을 잡는다. 진단 후 제거할 것.
	const float BeforeClamp = GetCurrentHealth();
	const float ClampedValue = FMath::Clamp(BeforeClamp, 0.0f, GetMaxHealth());
	if (!FMath::IsNearlyEqual(BeforeClamp, ClampedValue))
	{
		UE_LOG(LogLS, Warning,
			TEXT("[ChipStat][DEBUG] ClampCurrentHealth: %.0f -> %.0f (MaxHealth=%.0f)"),
			BeforeClamp, ClampedValue, GetMaxHealth());
	}
	SetCurrentHealth(ClampedValue);
}

void ULSCombatAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ULSCombatAttributeSet, MaxHealth, OldMaxHealth);
}

void ULSCombatAttributeSet::OnRep_CurrentHealth(const FGameplayAttributeData& OldCurrentHealth) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ULSCombatAttributeSet, CurrentHealth, OldCurrentHealth);
}
