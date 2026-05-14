#include "GAS/LSGameplayTags.h"

namespace LSGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG(State_SuperArmor, "LS.State.SuperArmor")
	UE_DEFINE_GAMEPLAY_TAG(State_Invincible, "LS.State.Invincible")
	UE_DEFINE_GAMEPLAY_TAG(State_Dead, "LS.State.Dead")
	UE_DEFINE_GAMEPLAY_TAG(State_Knockback, "LS.State.Knockback")

	UE_DEFINE_GAMEPLAY_TAG(Ability_Dash, "LS.Ability.Dash")
	UE_DEFINE_GAMEPLAY_TAG(Ability_PlayerBasicAttack, "LS.Ability.Player.BasicAttack")
	UE_DEFINE_GAMEPLAY_TAG(Ability_MonsterMelee, "LS.Ability.Monster.Melee")

	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Dash, "LS.Cooldown.Dash")

	UE_DEFINE_GAMEPLAY_TAG(Combat_Attacking, "LS.Combat.Attacking")
	UE_DEFINE_GAMEPLAY_TAG(Combat_AttackActive, "LS.Combat.AttackActive")
	UE_DEFINE_GAMEPLAY_TAG(Combat_ComboWindow, "LS.Combat.ComboWindow")

	UE_DEFINE_GAMEPLAY_TAG(Buff_CombatAcceleration, "LS.Buff.CombatAcceleration")

	UE_DEFINE_GAMEPLAY_TAG(Event_Combat_BasicAttackHit, "LS.Event.Combat.BasicAttackHit")

	UE_DEFINE_GAMEPLAY_TAG(Data_Damage_Fixed, "LS.Data.Damage.Fixed")
	UE_DEFINE_GAMEPLAY_TAG(Data_Damage_AttackCoefficient, "LS.Data.Damage.AttackCoefficient")
	UE_DEFINE_GAMEPLAY_TAG(Data_Damage_CanCrit, "LS.Data.Damage.CanCrit")

	UE_DEFINE_GAMEPLAY_TAG(Data_Buff_AttackSpeed, "LS.Data.Buff.AttackSpeed")
	UE_DEFINE_GAMEPLAY_TAG(Data_Buff_AttackPower, "LS.Data.Buff.AttackPower")
}
