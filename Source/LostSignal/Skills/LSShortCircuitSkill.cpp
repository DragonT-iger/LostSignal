#include "Skills/LSShortCircuitSkill.h"

#include "GameFramework/Pawn.h"
#include "GAS/Abilities/LSGA_ShortCircuit.h"
#include "LostSignal.h"
#include "Skills/LSShortCircuitField.h"
#include "Skills/LSShortCircuitProjectile.h"

ULSShortCircuitSkill::ULSShortCircuitSkill()
{
	DefaultAbilityClass = ULSGA_ShortCircuit::StaticClass();
	ProjectileClass = ALSShortCircuitProjectile::StaticClass();
	FieldClass = ALSShortCircuitField::StaticClass();
	AttackCoefficient = 1.5f;
}

bool ULSShortCircuitSkill::ActivateSkill_Implementation(const FLSSkillActivationContext& Context)
{
	UE_LOG(LogLS, Warning, TEXT("[ShortCircuit] ULSSkill activation is disabled. Use ULSShortCircuitSkillDataAsset + ULSGA_ShortCircuit."));
	return false;
}

TSubclassOf<ALSShortCircuitProjectile> ULSShortCircuitSkill::ResolveProjectileClass() const
{
	if (ProjectileClass)
	{
		return ProjectileClass;
	}

	return ALSShortCircuitProjectile::StaticClass();
}

TSubclassOf<ALSShortCircuitField> ULSShortCircuitSkill::ResolveFieldClass() const
{
	if (FieldClass)
	{
		return FieldClass;
	}

	return ALSShortCircuitField::StaticClass();
}
