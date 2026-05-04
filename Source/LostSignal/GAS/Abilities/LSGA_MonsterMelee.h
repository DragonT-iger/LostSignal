#pragma once

#include "Abilities/GameplayAbility.h"
#include "LSGA_MonsterMelee.generated.h"

class UAnimMontage;

/**
 * Minimal monster melee ability.
 * StateTree requests it, the ability plays a montage, and authored notifies drive hit timing.
 */
UCLASS()
class LOSTSIGNAL_API ULSGA_MonsterMelee : public UGameplayAbility
{
	GENERATED_BODY()

public:
	ULSGA_MonsterMelee();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

private:
	void HandleAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	/** Cached montage resolved from the owning monster character when the ability activates. */
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveAttackMontage;
};
