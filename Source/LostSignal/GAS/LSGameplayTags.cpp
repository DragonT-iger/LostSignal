#include "GAS/LSGameplayTags.h"

namespace LSGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG(State_SuperArmor, "LS.State.SuperArmor")
	UE_DEFINE_GAMEPLAY_TAG(State_Invincible, "LS.State.Invincible")
	UE_DEFINE_GAMEPLAY_TAG(State_Dead, "LS.State.Dead")

	UE_DEFINE_GAMEPLAY_TAG(Ability_Dash, "LS.Ability.Dash")
	UE_DEFINE_GAMEPLAY_TAG(Ability_MonsterMelee, "LS.Ability.Monster.Melee")

	UE_DEFINE_GAMEPLAY_TAG(Cooldown_Dash, "LS.Cooldown.Dash")

	UE_DEFINE_GAMEPLAY_TAG(Combat_Attacking, "LS.Combat.Attacking")
	UE_DEFINE_GAMEPLAY_TAG(Combat_AttackActive, "LS.Combat.AttackActive")

	UE_DEFINE_GAMEPLAY_TAG(Data_Damage_Base, "LS.Data.Damage.Base")
	UE_DEFINE_GAMEPLAY_TAG(Data_Damage_AttackCoefficient, "LS.Data.Damage.AttackCoefficient")
	UE_DEFINE_GAMEPLAY_TAG(Data_Damage_CanCrit, "LS.Data.Damage.CanCrit")
}
