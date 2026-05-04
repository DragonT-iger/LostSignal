#pragma once

#include "GameplayEffect.h"
#include "LSGE_MonsterBasicDamage.generated.h"

/**
 * Flat instant damage effect for the first monster melee vertical slice.
 */
UCLASS()
class LOSTSIGNAL_API ULSGE_MonsterBasicDamage : public UGameplayEffect
{
	GENERATED_BODY()

public:
	ULSGE_MonsterBasicDamage();
};
