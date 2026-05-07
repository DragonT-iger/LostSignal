#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "LSCharacterCombatComponent.generated.h"

class AActor;
class ALSCharacterBase;
class UAbilitySystemComponent;
class UGameplayEffect;
struct FOnAttributeChangeData;

UCLASS(ClassGroup=(LS), meta=(BlueprintSpawnableComponent))
class LOSTSIGNAL_API ULSCharacterCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSCharacterCombatComponent();

	UFUNCTION(BlueprintPure, Category="LS/Combat")
	ALSCharacterBase* GetOwnerCharacter() const;

	UFUNCTION(BlueprintPure, Category="LS/Combat")
	UAbilitySystemComponent* GetAbilitySystemComponent() const;

	UFUNCTION(BlueprintPure, Category="LS/Combat")
	bool HasCombatTag(FGameplayTag Tag) const;

	UFUNCTION(BlueprintPure, Category="LS/Combat")
	bool IsDead() const;

	UFUNCTION(BlueprintPure, Category="LS/Combat")
	bool CanStartAttack() const;

	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	void SetCombatTagActive(FGameplayTag Tag, bool bActive);

	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	bool ApplyDamageEffectToTarget(
		AActor* TargetActor,
		TSubclassOf<UGameplayEffect> DamageEffectClass,
		float EffectLevel = 1.0f,
		float BaseDamage = 0.0f,
		float AttackCoefficient = 0.0f,
		bool bCanCrit = false) const;

protected:
	virtual void BeginPlay() override;

private:
	void BindHealthDelegates();
	void HandleCurrentHealthChanged(const FOnAttributeChangeData& ChangeData);
	void RefreshDeathState();

	UPROPERTY()
	TMap<FGameplayTag, int32> LooseTagRefCounts;
};
