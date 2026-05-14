#pragma once

#include "GameplayEffect.h"
#include "LSGE_SkillCooldown.generated.h"

/** Generic skill cooldown GE. The applied spec adds the per-skill cooldown tag dynamically. */
UCLASS()
class LOSTSIGNAL_API ULSGE_SkillCooldown : public UGameplayEffect
{
	GENERATED_BODY()

public:
	ULSGE_SkillCooldown(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
