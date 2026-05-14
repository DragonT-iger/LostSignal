#pragma once

#include "GameplayEffect.h"
#include "LSGE_CombatAcceleration.generated.h"

/**
 * Stacking passive buff for Combat Acceleration.
 * SetByCaller values come from ULSCombatAccelerationSkillDataAsset.
 */
UCLASS()
class LOSTSIGNAL_API ULSGE_CombatAcceleration : public UGameplayEffect
{
	GENERATED_BODY()

public:
	ULSGE_CombatAcceleration(const FObjectInitializer& ObjectInitializer);
};
