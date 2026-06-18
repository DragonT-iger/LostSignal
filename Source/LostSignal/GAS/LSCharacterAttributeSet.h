#pragma once

#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "CoreMinimal.h"
#include "GAS/LSAttributeSetMacros.h"
#include "LSCharacterAttributeSet.generated.h"

// 캐릭터 능력치 전담 AttributeSet: 공격/방어/치명/관통/쿨감/이동/대시/스태미나.
// 체력(MaxHealth/CurrentHealth)은 ULSCombatAttributeSet이 보유한다.
UCLASS()
class LOSTSIGNAL_API ULSCharacterAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData Attack = 100.0f;
	LS_ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, Attack)

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData AttackSpeed = 1.0f;
	LS_ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, AttackSpeed)

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData AttackPowerMultiplier = 1.0f;
	LS_ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, AttackPowerMultiplier)

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData CooldownReduction = 0.0f;
	LS_ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, CooldownReduction)

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData CritChance = 0.0f;
	LS_ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, CritChance)

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData CritDamage = 1.5f;
	LS_ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, CritDamage)

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData ArmorPenetration = 0.0f;
	LS_ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, ArmorPenetration)

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData Defence = 0.0f;
	LS_ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, Defence)

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData DamageReduction = 0.0f;
	LS_ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, DamageReduction)

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData Recovery = 0.0f;
	LS_ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, Recovery)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MaxStamina, Category="LS/Stats")
	FGameplayAttributeData MaxStamina = 100.0f;
	LS_ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, MaxStamina)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_CurrentStamina, Category="LS/Stats")
	FGameplayAttributeData CurrentStamina = 100.0f;
	LS_ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, CurrentStamina)

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData MoveSpeed = 1.0f;
	LS_ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, MoveSpeed)

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData DashSpeed = 1200.0f;
	LS_ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, DashSpeed)

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData DashDuration = 0.3f;
	LS_ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, DashDuration)

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData DashCooldown = 1.0f;
	LS_ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, DashCooldown)

private:
	UFUNCTION()
	void OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina) const;

	UFUNCTION()
	void OnRep_CurrentStamina(const FGameplayAttributeData& OldCurrentStamina) const;

	void ClampCurrentStamina();
};
