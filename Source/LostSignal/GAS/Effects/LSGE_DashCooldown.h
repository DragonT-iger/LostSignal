#pragma once

#include "GameplayEffect.h"
#include "LSGE_DashCooldown.generated.h"

/**
 * 대쉬 쿨타임 GameplayEffect 베이스 클래스.
 * 실제 지속 시간은 에디터 Blueprint 에셋(GE_DashCooldown)에서 설정한다.
 *
 * 에디터 설정 방법:
 *   1. Content/LostSignal/Data/GAS/ 우클릭 → GameplayEffect → GE_DashCooldown
 *   2. Duration Policy    = Has Duration
 *   3. Duration Magnitude = Scalable Float, 값 1.0 (쿨타임 초, 기획자 조정)
 *   4. Gameplay Effect Components → Target Tags → Added: LS.Cooldown.Dash
 *   5. BP_GA_Dash의 Cooldown Gameplay Effect Class 슬롯에 GE_DashCooldown 할당
 *
 * 동작 원리:
 *   CommitAbility() → CooldownGameplayEffectClass 자동 적용
 *   → LS.Cooldown.Dash 태그 ASC에 부여
 *   → Duration 만료 시 GE 자동 제거 → 태그 소멸 → 쿨타임 해제
 */
UCLASS()
class LOSTSIGNAL_API ULSGE_DashCooldown : public UGameplayEffect
{
	GENERATED_BODY()
};
