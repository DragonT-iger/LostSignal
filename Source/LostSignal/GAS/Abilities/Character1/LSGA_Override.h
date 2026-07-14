#pragma once

#include "GAS/Abilities/Character1/LSGA_PlayerSkillBase.h"
#include "Combat/LSCombatTypes.h"
#include "LSGA_Override.generated.h"

class UGameplayEffect;
class ULSCharacterCombatComponent;

/**
 * GameplayAbility version of Override.
 * It consumes preview-confirm context from ULSPlayerSkillComponent, plays the skill montage,
 * then executes a self-centered shockwave at the LSAN_SkillEffect notify (몽타주 없으면 즉발).
 */
UCLASS()
class LOSTSIGNAL_API ULSGA_Override : public ULSGA_PlayerSkillBase
{
	GENERATED_BODY()

public:
	ULSGA_Override();

protected:
	virtual bool PrepareSkillExecution() override;
	virtual void ExecuteSkillEffect() override;

	UPROPERTY(EditDefaultsOnly, Category="LS/Skill|Override")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

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

	UPROPERTY(EditDefaultsOnly, Category="LS/Skill|Override|Debug")
	bool bEnableDebugLog = false;

private:
	// PrepareSkillExecution에서 검증·캐싱하여 ExecuteSkillEffect(노티파이 시점)에서 사용한다.
	UPROPERTY(Transient)
	TObjectPtr<ULSCharacterCombatComponent> CachedCombatComponent = nullptr;

	UPROPERTY(Transient)
	TSubclassOf<UGameplayEffect> CachedDamageEffectClass = nullptr;
};
