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
};
