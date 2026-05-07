#pragma once

#include "GameplayEffect.h"
#include "LSGE_DashInvincible.generated.h"

/**
 * 대쉬 무적 GameplayEffect.
 * C++ 생성자에서 Infinite Duration으로 초기화됨.
 * LS.State.Invincible 태그 그랜팅은 에디터에서 GE Components → Target Tags로 추가 필요.
 */
UCLASS()
class LOSTSIGNAL_API ULSGE_DashInvincible : public UGameplayEffect
{
	GENERATED_BODY()
public:
	ULSGE_DashInvincible(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
