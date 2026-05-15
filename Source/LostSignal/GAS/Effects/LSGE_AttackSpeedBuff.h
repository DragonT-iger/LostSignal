#pragma once

#include "GameplayEffect.h"
#include "LSGE_AttackSpeedBuff.generated.h"

/** Timed attack-speed-only buff. Magnitude is supplied through LS.Data.Buff.AttackSpeed. */
UCLASS()
class LOSTSIGNAL_API ULSGE_AttackSpeedBuff : public UGameplayEffect
{
	GENERATED_BODY()

public:
	ULSGE_AttackSpeedBuff(const FObjectInitializer& ObjectInitializer);
};
