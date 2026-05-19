#include "Skills/LSShortCircuitSkillDataAsset.h"

#include "GAS/Abilities/Character1/LSGA_ShortCircuit.h"
#include "GAS/LSGameplayTags.h"
#include "Skills/LSShortCircuitField.h"
#include "Skills/LSShortCircuitProjectile.h"

ULSShortCircuitSkillDataAsset::ULSShortCircuitSkillDataAsset()
{
	AbilityClass = ULSGA_ShortCircuit::StaticClass();
	CooldownTag = LSGameplayTags::Cooldown_Skill_ShortCircuit;
	ProjectileClass = ALSShortCircuitProjectile::StaticClass();
	FieldClass = ALSShortCircuitField::StaticClass();
	AttackCoefficient = 1.5f;
}

TSubclassOf<ALSShortCircuitProjectile> ULSShortCircuitSkillDataAsset::ResolveProjectileClass() const
{
	if (ProjectileClass)
	{
		return ProjectileClass;
	}

	return ALSShortCircuitProjectile::StaticClass();
}

TSubclassOf<ALSShortCircuitField> ULSShortCircuitSkillDataAsset::ResolveFieldClass() const
{
	if (FieldClass)
	{
		return FieldClass;
	}

	return ALSShortCircuitField::StaticClass();
}
