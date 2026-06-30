#include "GAS/Effects/LSGE_PlayerBasicDamage.h"

#include "GAS/Calculations/LSDamageExecutionCalculation.h"

ULSGE_PlayerBasicDamage::ULSGE_PlayerBasicDamage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayEffectExecutionDefinition DamageExecution;
	DamageExecution.CalculationClass = ULSDamageExecutionCalculation::StaticClass();
	Executions.Add(DamageExecution);
}
