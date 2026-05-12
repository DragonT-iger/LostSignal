#pragma once

#include "CoreMinimal.h"
#include "Skills/LSSkill.h"
#include "LSCombatAccelerationPassiveSkill.generated.h"

class UGameplayEffect;

/**
 * Passive skill: applies a stacking acceleration buff when the configured basic-attack combo hit succeeds.
 * Data comes from the assigned SkillRow/SkillDataAsset; the GameplayEffect asset owns the actual GAS modifiers.
 */
UCLASS(Blueprintable, BlueprintType)
class LOSTSIGNAL_API ULSCombatAccelerationPassiveSkill : public ULSSkill
{
	GENERATED_BODY()

public:
	virtual bool HandleBasicAttackHit_Implementation(const FLSBasicAttackHitContext& Context) const override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Passive")
	TSubclassOf<UGameplayEffect> BuffEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Passive")
	int32 RequiredComboIndex = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Passive", meta=(ClampMin="0.0"))
	float FallbackDuration = 15.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Passive", meta=(ClampMin="0.0"))
	float FallbackAttackSpeedBonus = 0.02f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Passive", meta=(ClampMin="0.0"))
	float FallbackAttackPowerBonus = 0.02f;
};
