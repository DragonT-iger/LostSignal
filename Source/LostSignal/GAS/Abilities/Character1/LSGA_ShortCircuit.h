#pragma once

#include "GAS/Abilities/Character1/LSGA_PlayerSkillBase.h"
#include "LSGA_ShortCircuit.generated.h"

class ALSShortCircuitProjectile;
class ULSShortCircuitSkillDataAsset;

/**
 * GameplayAbility version of Short Circuit projectile/field spawn.
 * 몽타주가 있으면 재생하고 LSAN_SkillEffect 노티파이 시점에, 없으면 즉발로 발사체를 스폰한다.
 */
UCLASS()
class LOSTSIGNAL_API ULSGA_ShortCircuit : public ULSGA_PlayerSkillBase
{
	GENERATED_BODY()

protected:
	virtual bool PrepareSkillExecution() override;
	virtual void ExecuteSkillEffect() override;

private:
	// PrepareSkillExecution에서 검증·캐싱하여 ExecuteSkillEffect(노티파이 시점)에서 사용한다.
	UPROPERTY(Transient)
	TObjectPtr<ULSShortCircuitSkillDataAsset> CachedShortCircuitData = nullptr;

	UPROPERTY(Transient)
	TSubclassOf<ALSShortCircuitProjectile> CachedProjectileClass = nullptr;
};
