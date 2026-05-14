#include "Skills/LSCombatAccelerationPassiveSkill.h"

#include "LostSignal.h"

bool ULSCombatAccelerationPassiveSkill::HandleBasicAttackHit_Implementation(const FLSBasicAttackHitContext& Context) const
{
	UE_LOG(LogLS, Warning, TEXT("ULSCombatAccelerationPassiveSkill is legacy-disabled. Use ULSCombatAccelerationSkillDataAsset + ULSGA_CombatAccelerationPassive."));
	return false;
}
