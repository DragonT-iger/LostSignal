#pragma once

#include "CoreMinimal.h"
#include "Skills/LSSkill.h"
#include "LSOverrideSkill.generated.h"

/**
 * Active skill: self-centered shockwave that deals light damage and briefly pushes enemies away.
 * Upgrade rows are intentionally left for the later skill-upgrade system.
 */
UCLASS(Blueprintable, BlueprintType)
class LOSTSIGNAL_API ULSOverrideSkill : public ULSSkill
{
	GENERATED_BODY()

public:
	ULSOverrideSkill();

	virtual bool ActivateSkill_Implementation(const FLSSkillActivationContext& Context) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Override", meta=(ClampMin="0.0"))
	float FallbackRadius = 450.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Override", meta=(ClampMin="0.0"))
	float FallbackAttackCoefficient = 1.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Override", meta=(ClampMin="0.0"))
	float KnockbackSpeed = 650.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Override")
	float KnockbackUpSpeed = 80.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Override|Debug")
	bool bEnableDebugLog = false;
};
