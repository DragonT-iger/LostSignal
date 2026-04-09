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
	/** 대쉬(구르기) 중인 상태 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dodging)

	/** 무적 상태 — 대쉬 중 DatamiGE가 부여 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Invincible)

	// ── 어빌리티 태그 ─────────────────────────────────────────
	/** 대쉬 어빌리티 식별 태그 */
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Dash)
}
