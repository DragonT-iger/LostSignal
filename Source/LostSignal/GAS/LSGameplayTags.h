#pragma once

#include "NativeGameplayTags.h"

/**
 * Central GameplayTag declarations used by LostSignal C++ code.
 * Keep tags here when they affect GAS, StateTree, or combat flow.
 */
namespace LSGameplayTags
{
	// Shared state tags
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Invincible)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dead)

	// Ability tags
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Dash)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_MonsterMelee)

	// Cooldown tags
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Dash)

	// Combat phase tags
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_Attacking)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_AttackActive)
}
