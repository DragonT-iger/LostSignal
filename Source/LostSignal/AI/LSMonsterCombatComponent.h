#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "LSMonsterCombatComponent.generated.h"

struct FLSMonsterArchetypeRow;

/** Thin bridge from monster AI to GAS ability activation. */
UCLASS(ClassGroup=(LS), meta=(BlueprintSpawnableComponent))
class LOSTSIGNAL_API ULSMonsterCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSMonsterCombatComponent();

	void ApplyArchetype(const FLSMonsterArchetypeRow& Row);

	UFUNCTION(BlueprintPure, Category="AI|Combat")
	FGameplayTag GetDefaultAttackAbilityTag() const { return DefaultAttackAbilityTag; }

	UFUNCTION(BlueprintPure, Category="AI|Combat")
	float GetLeashDistance() const { return LeashDistance; }

	UFUNCTION(BlueprintPure, Category="AI|Combat")
	float GetAlertMoveSpeedMultiplier() const { return AlertMoveSpeedMultiplier; }

	UFUNCTION(BlueprintCallable, Category="AI|Combat")
	bool RequestAbilityByTag(FGameplayTag AbilityTag) const;

private:
	UPROPERTY(EditAnywhere, Category="Combat")
	FGameplayTag DefaultAttackAbilityTag;

	UPROPERTY(EditAnywhere, Category="Combat", meta=(ClampMin="0.0"))
	float LeashDistance = 2000.0f;

	UPROPERTY(EditAnywhere, Category="Combat", meta=(ClampMin="0.0"))
	float AlertMoveSpeedMultiplier = 1.2f;
};
