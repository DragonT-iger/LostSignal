#include "GAS/Effects/LSGE_MonsterBasicDamage.h"

#include "GAS/Calculations/LSDamageExecutionCalculation.h"
#include "GAS/LSGameplayTags.h"

ULSGE_MonsterBasicDamage::ULSGE_MonsterBasicDamage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FGameplayEffectExecutionDefinition DamageExecution;
	DamageExecution.CalculationClass = ULSDamageExecutionCalculation::StaticClass();
	Executions.Add(DamageExecution);

	// 대상에 적용되는 순간 피격 GameplayCue 발동(피격자 위치에서 피격음 재생).
	FGameplayEffectCue HitCue;
	HitCue.GameplayCueTags.AddTag(LSGameplayTags::GameplayCue_Combat_Hit);
	GameplayCues.Add(HitCue);
}
