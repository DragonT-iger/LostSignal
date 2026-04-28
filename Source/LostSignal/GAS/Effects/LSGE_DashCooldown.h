#pragma once

#include "GameplayEffect.h"
#include "LSGE_DashCooldown.generated.h"

/**
 * 대쉬 쿨타임 GameplayEffect.
 * C++ 생성자에서 Duration 1초로 초기화됨.
 * LS.Cooldown.Dash 태그 그랜팅은 에디터에서 GE Components → Target Tags로 추가 필요.
 */
UCLASS()
class LOSTSIGNAL_API ULSGE_DashCooldown : public UGameplayEffect
{
	GENERATED_BODY()
public:
	ULSGE_DashCooldown();
};
