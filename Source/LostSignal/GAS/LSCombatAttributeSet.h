#pragma once

#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "CoreMinimal.h"
#include "GAS/LSAttributeSetMacros.h"
#include "LSCombatAttributeSet.generated.h"

// 생명력 풀 전담 AttributeSet: MaxHealth / CurrentHealth + 데미지 적용용 임시 Damage.
// 공격/방어/치명/관통/쿨감/이동/대시/스태미나 같은 능력치는 ULSCharacterAttributeSet이 보유한다.
UCLASS()
class LOSTSIGNAL_API ULSCombatAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_MaxHealth, Category="LS/Stats")
	FGameplayAttributeData MaxHealth = 1000.0f;
	LS_ATTRIBUTE_ACCESSORS(ULSCombatAttributeSet, MaxHealth)

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_CurrentHealth, Category="LS/Stats")
	FGameplayAttributeData CurrentHealth = 1000.0f;
	LS_ATTRIBUTE_ACCESSORS(ULSCombatAttributeSet, CurrentHealth)

	UPROPERTY(BlueprintReadOnly, Category="LS/Stats")
	FGameplayAttributeData Damage = 0.0f;
	LS_ATTRIBUTE_ACCESSORS(ULSCombatAttributeSet, Damage)

private:
	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth) const;

	UFUNCTION()
	void OnRep_CurrentHealth(const FGameplayAttributeData& OldCurrentHealth) const;

	void ClampCurrentHealth();
};
