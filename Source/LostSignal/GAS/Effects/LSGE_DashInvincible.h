#pragma once

#include "GameplayEffect.h"
#include "LSGE_DashInvincible.generated.h"

/**
 * 대쉬 무적 GameplayEffect 베이스 클래스.
 * 실제 설정(Duration, Granted Tags)은 에디터에서 Blueprint 에셋으로 구성한다.
 *
 * 에디터 설정 방법:
 *   1. Content/LostSignal/Data/GAS/ 에서 우클릭 → GameplayEffect → GE_DashInvincible
 *   2. Duration Policy = Infinite
 *   3. Gameplay Effect Components → Target Tags → Added: LS.State.Invincible, LS.State.Dodging
 *   4. BP_GA_Dash의 InvincibilityEffectClass 슬롯에 GE_DashInvincible 할당
 */
UCLASS()
class LOSTSIGNAL_API ULSGE_DashInvincible : public UGameplayEffect
{
	GENERATED_BODY()
};
