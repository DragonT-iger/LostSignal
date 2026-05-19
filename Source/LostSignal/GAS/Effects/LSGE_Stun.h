#pragma once

#include "GameplayEffect.h"
#include "LSGE_Stun.generated.h"

/** Duration GameplayEffect that grants LS.State.Stunned. Duration can be overridden on the outgoing spec. */
UCLASS()
class LOSTSIGNAL_API ULSGE_Stun : public UGameplayEffect
{
	GENERATED_BODY()

public:
	ULSGE_Stun(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
