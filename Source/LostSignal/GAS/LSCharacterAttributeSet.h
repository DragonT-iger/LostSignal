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
 * 캐릭터 공통 스탯 AttributeSet.
 * 기획자 엑셀 Character_stat 컬럼과 1:1 대응.
 */
UCLASS()
class LOSTSIGNAL_API ULSCharacterAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	// 공격력
	UPROPERTY(BlueprintReadOnly, Category="Stats")
	FGameplayAttributeData Attack;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, Attack)

	// 공격속도
	UPROPERTY(BlueprintReadOnly, Category="Stats")
	FGameplayAttributeData AttackSpeed;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, AttackSpeed)

	// 가속력(스킬 쿨타임 관련)
	UPROPERTY(BlueprintReadOnly, Category="Stats")
	FGameplayAttributeData CooldownReduction;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, CooldownReduction)

	// 치명타 확률
	UPROPERTY(BlueprintReadOnly, Category="Stats")
	FGameplayAttributeData CritChance;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, CritChance)

	// 치명타 배율
	UPROPERTY(BlueprintReadOnly, Category="Stats")
	FGameplayAttributeData CritDamage;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, CritDamage)

	// 방관통
	UPROPERTY(BlueprintReadOnly, Category="Stats")
	FGameplayAttributeData ArmorPenetration;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, ArmorPenetration)

	// 최대 체력
	UPROPERTY(BlueprintReadOnly, Category="Stats")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, MaxHealth)

	// 방어
	UPROPERTY(BlueprintReadOnly, Category="Stats")
	FGameplayAttributeData Defence;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, Defence)

	// 자연회복량
	UPROPERTY(BlueprintReadOnly, Category="Stats")
	FGameplayAttributeData Recovery;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, Recovery)

	// 최대 스태미나
	UPROPERTY(BlueprintReadOnly, Category="Stats")
	FGameplayAttributeData MaxStamina;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, MaxStamina)

	// 이동속도
	UPROPERTY(BlueprintReadOnly, Category="Stats")
	FGameplayAttributeData MoveSpeed;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, MoveSpeed)
};
