#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "LSSTCondition_IsKnockback.generated.h"

/** Input payload for checking the knockback state mirrored by the monster evaluator. */
USTRUCT()
struct FLSSTCondition_IsKnockbackInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="LS/AI")
	bool bIsKnockback = false;

	UPROPERTY(EditAnywhere, Category="LS/AI")
	bool bInvert = false;
};

/** StateTree condition used for transitions into or out of temporary knockback CC. */
USTRUCT(meta=(DisplayName="LS Is Knockback", Category="LS|AI"))
struct LOSTSIGNAL_API FLSSTCondition_IsKnockback : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLSSTCondition_IsKnockbackInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};
