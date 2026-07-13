#pragma once

#include "GameplayEffect.h"
#include "LSGE_FullHeal.generated.h"

/** Instant full heal: 대상의 MaxHealth로 CurrentHealth를 덮어쓴다. 몬스터 복귀 시 전투 리셋에 사용. */
UCLASS()
class LOSTSIGNAL_API ULSGE_FullHeal : public UGameplayEffect
{
	GENERATED_BODY()

public:
	ULSGE_FullHeal(const FObjectInitializer& ObjectInitializer);
};
