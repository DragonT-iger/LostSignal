#pragma once

#include "GAS/Abilities/Character1/LSGA_PlayerSkillBase.h"
#include "Combat/LSCombatTypes.h"
#include "LSGA_Overclock.generated.h"

class UGameplayEffect;
class ULSCharacterCombatComponent;

/**
 * GameplayAbility version of Overclock.
 * 몽타주가 있으면 재생하고 LSAN_SkillEffect 노티파이 시점에, 없으면 즉발로 전방 콘 범위 타격을 실행한다.
 */
UCLASS()
class LOSTSIGNAL_API ULSGA_Overclock : public ULSGA_PlayerSkillBase
{
	GENERATED_BODY()

public:
	ULSGA_Overclock();

protected:
	virtual bool PrepareSkillExecution() override;
	virtual void ExecuteSkillEffect() override;

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

	// PrepareSkillExecution에서 검증·캐싱하여 ExecuteSkillEffect(노티파이 시점)에서 사용한다.
	UPROPERTY(Transient)
	TObjectPtr<ULSCharacterCombatComponent> CachedCombatComponent = nullptr;

	UPROPERTY(Transient)
	TSubclassOf<UGameplayEffect> CachedDamageEffectClass = nullptr;
};
