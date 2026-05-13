#pragma once

#include "CoreMinimal.h"
#include "Skills/LSSkill.h"
#include "LSOverclockSkill.generated.h"

/**
 * Active skill: frontal cone slash that consumes Combat Acceleration stacks for bonus damage.
 * Data is resolved from the assigned SkillRow through ULSSkillDataAsset.
 */
UCLASS(Blueprintable, BlueprintType)
class LOSTSIGNAL_API ULSOverclockSkill : public ULSSkill
{
	GENERATED_BODY()

public:
	ULSOverclockSkill();

	virtual bool ActivateSkill_Implementation(const FLSSkillActivationContext& Context) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Overclock", meta=(ClampMin="0.0"))
	float FallbackRange = 300.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Overclock", meta=(ClampMin="0.0", ClampMax="360.0"))
	float FallbackConeDegrees = 120.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Overclock", meta=(ClampMin="0.0"))
	float FallbackAdditionalAttackCoefficientPerStack = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Overclock|Debug")
	bool bEnableDebugVisualization = false;

private:
	int32 ConsumeCombatAccelerationStacks(AActor* SourceActor) const;
};
