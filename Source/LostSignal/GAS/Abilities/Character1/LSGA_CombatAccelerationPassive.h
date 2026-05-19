#pragma once

#include "Abilities/GameplayAbility.h"
#include "LSGA_CombatAccelerationPassive.generated.h"

/**
 * Passive GameplayAbility for Combat Acceleration.
 * Triggered by LS.Event.Combat.BasicAttackHit and applies the stacking buff through GAS.
 */
UCLASS()
class LOSTSIGNAL_API ULSGA_CombatAccelerationPassive : public UGameplayAbility
{
	GENERATED_BODY()

public:
	ULSGA_CombatAccelerationPassive();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
