#pragma once

#include "NativeGameplayTags.h"

/**
 * Central GameplayTag declarations used by LostSignal C++ code.
 * Keep tags here when they affect GAS, StateTree, or combat flow.
 */
namespace LSGameplayTags
{
	// Shared state tags
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_SuperArmor)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Invincible)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Dead)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Knockback)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(State_Stunned)

	// Ability tags
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Dash)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_PlayerBasicAttack)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_MonsterMelee)

	// Cooldown tags
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Dash)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Skill_Overclock)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Skill_Bypass)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Skill_ShortCircuit)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Skill_Override)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Skill_Execution)

	// Combat phase tags
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_Attacking)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_AttackActive)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_ComboWindow)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_NextAttack_ComboIndexOverride)

	// Buff tags
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Buff_CombatAcceleration)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Buff_AttackSpeed)

	// Gameplay event tags
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Combat_BasicAttackHit)

	// Gameplay noise tags
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Noise_Idle)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Noise_Walk)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Noise_Run)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Noise_Interact)

	// SetByCaller damage data
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Damage_Fixed)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Damage_AttackCoefficient)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Damage_CanCrit)

	// SetByCaller buff data
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Buff_AttackSpeed)
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Buff_AttackPower)

	// SetByCaller stamina data
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Stamina_Amount)
}
