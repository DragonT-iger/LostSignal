#pragma once

#include "CoreMinimal.h"
#include "Skills/LSSkillDataAsset.h"
#include "LSExecutionSkillDataAsset.generated.h"

/**
 * Execution-specific skill data.
 * Keep per-skill projectile/field/explosion values here instead of bloating ULSSkillDataAsset.
 */
UCLASS(BlueprintType)
class LOSTSIGNAL_API ULSExecutionSkillDataAsset : public ULSSkillDataAsset
{
	GENERATED_BODY()

public:
	ULSExecutionSkillDataAsset();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Execution", meta=(ClampMin="0.0"))
	float FallbackDashDistance = 650.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Execution", meta=(ClampMin="0.0"))
	float FallbackDashDuration = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Execution", meta=(ClampMin="0.0"))
	float FallbackSlashWidth = 220.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Execution", meta=(ClampMin="0.0"))
	float AdditionalAttackCoefficientPerAccelerationStack = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Execution|ShortCircuit", meta=(ClampMin="0.0"))
	float FieldExplosionAttackCoefficient = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Execution|ShortCircuit", meta=(ClampMin="0.0"))
	float FieldExplosionRadiusOverride = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Execution|ShortCircuit")
	bool bDestroyShortCircuitFieldOnExplosion = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Execution|Debug")
	bool bEnableDebugLog = false;
};
