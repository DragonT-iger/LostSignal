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
	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData Attack;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, Attack)

	// 공격속도
	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData AttackSpeed;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, AttackSpeed)

	// 스킬가속(스킬 쿨타임 관련)
	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData CooldownReduction;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, CooldownReduction)

	// 치명타 확률
	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData CritChance;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, CritChance)

	// 치명타 배율
	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData CritDamage;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, CritDamage)

	// 방관통
	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData ArmorPenetration;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, ArmorPenetration)

	// 최대 체력
	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, MaxHealth)

	// 최대 체력
	UPROPERTY(BlueprintReadOnly, Category = "LS/Stats")
	FGameplayAttributeData CurrentHealth;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, CurrentHealth)

	// 방어
	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData Defence;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, Defence)

	// 자연회복량
	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData Recovery;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, Recovery)

	// 최대 스태미나
	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData MaxStamina;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, MaxStamina)

	// 이동속도
	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData MoveSpeed;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, MoveSpeed)

	// 대쉬속도
	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData DashSpeed;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, DashSpeed)

	// 대쉬 지속시간 (초)
	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData DashDuration;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, DashDuration)

	// 대쉬 쿨타임 (초)
	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData DashCooldown;
	ATTRIBUTE_ACCESSORS(ULSCharacterAttributeSet, DashCooldown)
};
