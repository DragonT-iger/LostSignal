#pragma once

#include "GameplayEffect.h"
#include "LSGE_TrainingDummyRecovery.generated.h"

/** 허수아비의 Recovery(HP/s)를 1초마다 CurrentHealth에 더하는 무한 지속 GameplayEffect. */
UCLASS()
class LOSTSIGNAL_API ULSGE_TrainingDummyRecovery : public UGameplayEffect
{
	GENERATED_BODY()

public:
	ULSGE_TrainingDummyRecovery(const FObjectInitializer& ObjectInitializer);
};
