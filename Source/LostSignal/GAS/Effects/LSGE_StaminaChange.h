#pragma once

#include "GameplayEffect.h"
#include "LSGE_StaminaChange.generated.h"

/** Instant stamina delta. Magnitude is supplied through LS.Data.Stamina.Amount. */
UCLASS()
class LOSTSIGNAL_API ULSGE_StaminaChange : public UGameplayEffect
{
	GENERATED_BODY()

public:
	ULSGE_StaminaChange(const FObjectInitializer& ObjectInitializer);
};
