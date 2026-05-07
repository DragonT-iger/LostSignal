#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "LSMonsterCombatComponent.generated.h"

class UGameplayEffect;
class UAnimMontage;
struct FLSMonsterArchetypeRow;

/**
 * Thin bridge from monster AI to GAS ability activation and hit application.
 * StateTree asks for an attack by tag; animation notifies call PerformMeleeHit().
 */
UCLASS(ClassGroup=(LS), meta=(BlueprintSpawnableComponent))
class LOSTSIGNAL_API ULSMonsterCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSMonsterCombatComponent();

	void ApplyArchetype(const FLSMonsterArchetypeRow& Row);

	UFUNCTION(BlueprintPure, Category="LS/Combat")
	FGameplayTag GetDefaultAttackAbilityTag() const { return DefaultAttackAbilityTag; }

	UFUNCTION(BlueprintPure, Category="LS/Combat")
	float GetLeashDistance() const { return LeashDistance; }

	UFUNCTION(BlueprintPure, Category="LS/Combat")
	float GetAlertMoveSpeedMultiplier() const { return AlertMoveSpeedMultiplier; }

	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	bool RequestAbilityByTag(FGameplayTag AbilityTag) const;

	/** Called from an authored attack notify to apply the real melee hit on that frame. */
	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	void PerformMeleeHit();

	UFUNCTION(BlueprintPure, Category="LS/Combat")
	bool HasValidDamageEffect() const;

private:
	UPROPERTY(EditAnywhere, Category="LS/Combat")
	FGameplayTag DefaultAttackAbilityTag;

	UPROPERTY(EditAnywhere, Category="LS/Combat", meta=(ClampMin="0.0"))
	float LeashDistance = 2000.0f;

	UPROPERTY(EditAnywhere, Category="LS/Combat", meta=(ClampMin="0.0"))
	float AlertMoveSpeedMultiplier = 1.2f;

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat", meta=(ClampMin="0.0"))
	float MeleeHitForwardOffset = 110.0f;

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat", meta=(ClampMin="0.0"))
	float MeleeHitRadius = 90.0f;

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat", meta=(ClampMin="1.0"))
	float DamageEffectLevel = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat", meta=(ClampMin="0.0"))
	float MeleeBaseDamage = 10.0f;

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat", meta=(ClampMin="0.0"))
	float MeleeAttackCoefficient = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat")
	bool bMeleeCanCrit = false;
};
