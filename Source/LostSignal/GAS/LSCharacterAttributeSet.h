#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "LSCharacterAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName)           \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName)               \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName)               \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * Shared character attribute set for early LostSignal combat slices.
 * CurrentHealth and MaxHealth are split so damage can modify current HP cleanly.
 */
UCLASS()
class LOSTSIGNAL_API ULSCharacterAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData Attack;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, Attack)

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData AttackSpeed;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, AttackSpeed)

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData CooldownReduction;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, CooldownReduction)

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData CritChance;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, CritChance)

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData CritDamage;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, CritDamage)

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData ArmorPenetration;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, ArmorPenetration)

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, MaxHealth)

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData CurrentHealth;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, CurrentHealth)

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData Defence;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, Defence)

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData Recovery;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, Recovery)

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData MaxStamina;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, MaxStamina)

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData MoveSpeed;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, MoveSpeed)

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData DashSpeed;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, DashSpeed)

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData DashDuration;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, DashDuration)

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData DashCooldown;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, DashCooldown)

private:
	void ClampCurrentHealth();
};
