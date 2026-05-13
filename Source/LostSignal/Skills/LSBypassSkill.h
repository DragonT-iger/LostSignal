#pragma once

#include "CoreMinimal.h"
#include "Skills/LSSkill.h"
#include "LSBypassSkill.generated.h"

/**
 * Active skill: quick forward bypass movement.
 * Upgrade rows are intentionally not handled here yet; they will be resolved by the upgrade system later.
 */
UCLASS(Blueprintable, BlueprintType)
class LOSTSIGNAL_API ULSBypassSkill : public ULSSkill
{
	GENERATED_BODY()

public:
	ULSBypassSkill();

	virtual bool ActivateSkill_Implementation(const FLSSkillActivationContext& Context) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Bypass", meta=(ClampMin="0.0"))
	float FallbackDistance = 600.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Bypass", meta=(ClampMin="0.0"))
	float FallbackDuration = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Bypass|Debug")
	bool bEnableDebugLog = false;

private:
	void ClearBypassInvincibleTag(AActor* SourceActor) const;
};
