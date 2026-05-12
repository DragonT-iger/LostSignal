#pragma once

#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "CoreMinimal.h"
#include "LSCharacterAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName)           \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName)               \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName)               \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class LOSTSIGNAL_API ULSCharacterAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData Attack = 100.0f;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, Attack)

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData AttackSpeed = 1.0f;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, AttackSpeed)

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData AttackPowerMultiplier = 1.0f;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, AttackPowerMultiplier)

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData CooldownReduction = 0.0f;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, CooldownReduction)

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData CritChance = 0.0f;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, CritChance)

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData CritDamage = 1.5f;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, CritDamage)

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData ArmorPenetration = 0.0f;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, ArmorPenetration)

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData Defence = 0.0f;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, Defence)

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData Recovery = 0.0f;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, Recovery)

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData MaxStamina = 100.0f;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, MaxStamina)

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData MoveSpeed = 1.0f;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, MoveSpeed)

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData DashSpeed = 1200.0f;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, DashSpeed)

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData DashDuration = 0.3f;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, DashDuration)

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData DashCooldown = 1.0f;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, DashCooldown)
};
