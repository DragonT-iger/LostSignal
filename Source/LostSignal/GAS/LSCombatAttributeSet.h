#pragma once

#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "CoreMinimal.h"
#include "LSCombatAttributeSet.generated.h"

#define LS_ATTRIBUTE_ACCESSORS(ClassName, PropertyName)        \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName)               \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName)               \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class LOSTSIGNAL_API ULSCombatAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData MaxHealth = 100.0f;
	LS_ATTRIBUTE_ACCESSORS(ULSCombatAttributeSet, MaxHealth)

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData CurrentHealth = 100.0f;
	LS_ATTRIBUTE_ACCESSORS(ULSCombatAttributeSet, CurrentHealth)

private:
	void ClampCurrentHealth();
};
