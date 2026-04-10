#pragma once

#include "NativeGameplayTags.h"

/**
 * LostSignal 전역 GameplayTag 선언.
 * Unity의 string tag 대신 컴파일 타임에 검증되는 타입 안전 태그.
 *
 * 사용 예:
 *   ASC->HasMatchingGameplayTag(LSGameplayTags::State_Invincible)
 */
namespace LSGameplayTags
{
	// ── 상태 태그 ─────────────────────────────────────────────
	/** 무적 상태 — 대쉬 중 GE가 부여. 재발동 차단 태그로도 사용 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Invincible)

	// ── 어빌리티 태그 ─────────────────────────────────────────
	/** 대쉬 어빌리티 식별 태그 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Dash)

	// ── 쿨타임 태그 ───────────────────────────────────────────
	/** 대쉬 쿨타임 중 ASC에 부여되는 태그.
	 *  GE_DashCooldown이 활성화된 동안 존재 → CommitAbility가 이 태그로 재발동 차단. */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Dash)
}
