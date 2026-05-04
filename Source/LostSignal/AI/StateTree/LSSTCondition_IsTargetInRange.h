#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "LSSTCondition_IsTargetInRange.generated.h"

/** Input payload for checking whether the current target is close enough to attack. */
USTRUCT()
struct FLSSTCondition_IsTargetInRangeInstanceData
{
	GENERATED_BODY()

	/** Runtime distance value usually bound from the monster sense evaluator. */
	UPROPERTY(EditAnywhere, Category="LS/AI")
	float DistanceToTarget = 0.0f;

	/** Designer-authored threshold for entering the attack state. */
	UPROPERTY(EditAnywhere, Category="LS/AI", meta=(ClampMin="0.0"))
	float RequiredRange = 150.0f;

	/** Optional inversion for transitions like "leave attack when target is out of range". */
	UPROPERTY(EditAnywhere, Category="LS/AI")
	bool bInvert = false;
};

/** StateTree condition used for range-gated combat transitions such as Combat -> Attack. */
USTRUCT(meta=(DisplayName="LS Is Target In Range", Category="LS|AI"))
struct LOSTSIGNAL_API FLSSTCondition_IsTargetInRange : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLSSTCondition_IsTargetInRangeInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};
