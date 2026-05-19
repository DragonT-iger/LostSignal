#pragma once

#include "Abilities/GameplayAbility.h"
#include "LSGA_ShortCircuit.generated.h"

/** GameplayAbility version of Short Circuit projectile/field spawn. */
UCLASS()
class LOSTSIGNAL_API ULSGA_ShortCircuit : public UGameplayAbility
{
	GENERATED_BODY()

public:
	ULSGA_ShortCircuit();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
