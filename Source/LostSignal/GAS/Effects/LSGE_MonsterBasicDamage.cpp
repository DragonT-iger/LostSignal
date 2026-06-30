#include "GAS/Effects/LSGE_MonsterBasicDamage.h"

#include "GAS/Calculations/LSDamageExecutionCalculation.h"

ULSGE_MonsterBasicDamage::ULSGE_MonsterBasicDamage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayEffectExecutionDefinition DamageExecution;
	DamageExecution.CalculationClass = ULSDamageExecutionCalculation::StaticClass();
	Executions.Add(DamageExecution);
}
