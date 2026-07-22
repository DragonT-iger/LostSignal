#pragma once

#include "GameplayEffect.h"
#include "LSGE_HealthChange.generated.h"

/** Instant health delta. Magnitude is supplied through LS.Data.Health.Amount. */
UCLASS()
class LOSTSIGNAL_API ULSGE_HealthChange : public UGameplayEffect
{
	GENERATED_BODY()

public:
	ULSGE_HealthChange(const FObjectInitializer& ObjectInitializer);
};
