#pragma once

#include "CoreMinimal.h"
#include "Skills/LSPassiveSkillDataAsset.h"
#include "LSCombatAccelerationSkillDataAsset.generated.h"

class UGameplayEffect;

/** Passive data for Combat Acceleration. Application is handled by ULSPlayerSkillComponent. */
UCLASS(BlueprintType)
class LOSTSIGNAL_API ULSCombatAccelerationSkillDataAsset : public ULSPassiveSkillDataAsset
{
	GENERATED_BODY()

public:
	ULSCombatAccelerationSkillDataAsset();

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
