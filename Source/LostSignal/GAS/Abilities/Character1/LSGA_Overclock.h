#pragma once

#include "Abilities/GameplayAbility.h"
#include "Combat/LSCombatTypes.h"
#include "LSGA_Overclock.generated.h"

class UGameplayEffect;

/** GameplayAbility version of Overclock. */
UCLASS()
class LOSTSIGNAL_API ULSGA_Overclock : public UGameplayAbility
{
	GENERATED_BODY()

public:
	ULSGA_Overclock();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	UPROPERTY(EditDefaultsOnly, Category="LS/Skill|Overclock")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, Category="LS/Skill|Overclock", meta=(ClampMin="0.0"))
	float FallbackRange = 300.0f;

	UPROPERTY(EditDefaultsOnly, Category="LS/Skill|Overclock", meta=(ClampMin="0.0", ClampMax="360.0"))
	float FallbackConeDegrees = 120.0f;

	UPROPERTY(EditDefaultsOnly, Category="LS/Skill|Overclock", meta=(ClampMin="0.0"))
	float FallbackAttackCoefficient = 2.5f;

	UPROPERTY(EditDefaultsOnly, Category="LS/Skill|Overclock", meta=(ClampMin="0.0"))
	float FallbackAdditionalAttackCoefficientPerStack = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category="LS/Skill|Overclock")
	bool bCanCrit = false;

	UPROPERTY(EditDefaultsOnly, Category="LS/Skill|Overclock")
	ELSBreakPowerTier BreakPower = ELSBreakPowerTier::NormalAttack;

	UPROPERTY(EditDefaultsOnly, Category="LS/Skill|Overclock|Debug")
	bool bEnableDebugLog = false;

private:
	int32 ConsumeCombatAccelerationStacks(AActor* SourceActor) const;
};
