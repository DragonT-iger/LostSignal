#pragma once

#include "GameplayEffect.h"
#include "LSGE_DashCooldown.generated.h"

/**
 * 대쉬 쿨타임 GameplayEffect.
 * C++ 생성자에서 Duration 1초로 초기화됨.
 * LS.Cooldown.Dash 태그를 Grant하여 LSGA_Dash::GetCooldownTags()의 재발동 차단에 사용.
 */
UCLASS()
class LOSTSIGNAL_API ULSGE_DashCooldown : public UGameplayEffect
{
	GENERATED_BODY()
public:
	ULSGE_DashCooldown(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
