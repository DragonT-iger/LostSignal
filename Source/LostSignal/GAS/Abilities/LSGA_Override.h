#pragma once

#include "Abilities/GameplayAbility.h"
#include "Combat/LSCombatTypes.h"
#include "LSGA_Override.generated.h"

class UGameplayEffect;

/**
 * GameplayAbility version of Override.
 * It consumes preview-confirm context from ULSPlayerSkillComponent, then executes a self-centered shockwave.
 */
UCLASS()
class LOSTSIGNAL_API ULSGA_Override : public UGameplayAbility
{
	GENERATED_BODY()

public:
	ULSGA_Override();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	UPROPERTY(EditDefaultsOnly, Category="LS/Skill|Override")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, Category="LS/Skill|Override", meta=(ClampMin="0.0"))
	float FixedDamage = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category="LS/Skill|Override", meta=(ClampMin="0.0"))
	float FallbackRadius = 450.0f;

	UPROPERTY(EditDefaultsOnly, Category="LS/Skill|Override", meta=(ClampMin="0.0"))
	float FallbackAttackCoefficient = 1.2f;

	UPROPERTY(EditDefaultsOnly, Category="LS/Skill|Override")
	bool bCanCrit = false;

	UPROPERTY(EditDefaultsOnly, Category="LS/Skill|Override")
	ELSBreakPowerTier BreakPower = ELSBreakPowerTier::NormalAttack;

	UPROPERTY(EditDefaultsOnly, Category="LS/Skill|Override", meta=(ClampMin="0.0"))
	float KnockbackSpeed = 650.0f;

	UPROPERTY(EditDefaultsOnly, Category="LS/Skill|Override")
	float KnockbackUpSpeed = 80.0f;

	UPROPERTY(EditDefaultsOnly, Category="LS/Skill|Override|Debug")
	bool bEnableDebugLog = false;
};
