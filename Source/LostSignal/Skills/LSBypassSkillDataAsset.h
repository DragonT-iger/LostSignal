#pragma once

#include "CoreMinimal.h"
#include "Skills/LSSkillDataAsset.h"
#include "LSBypassSkillDataAsset.generated.h"

/** Bypass-specific data. Macro variants can reserve the next basic attack combo index here. */
UCLASS(BlueprintType)
class LOSTSIGNAL_API ULSBypassSkillDataAsset : public ULSSkillDataAsset
{
	GENERATED_BODY()

public:
	ULSBypassSkillDataAsset();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Bypass", meta=(ClampMin="0.0"))
	float FallbackDistance = 600.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Bypass", meta=(ClampMin="0.0"))
	float FallbackDuration = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Bypass|Macro")
	bool bSetComboIndexOverrideOnFinish = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Bypass|Macro", meta=(ClampMin="0"))
	int32 ComboIndexOverride = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Bypass|Macro", meta=(ClampMin="0.0"))
	float ComboIndexOverrideWindowSeconds = 1.0f;
};
