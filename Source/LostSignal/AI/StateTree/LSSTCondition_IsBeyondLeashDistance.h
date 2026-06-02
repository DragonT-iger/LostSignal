#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "LSSTCondition_IsBeyondLeashDistance.generated.h"

/** Input payload for checking whether the monster is outside its leash distance. */
USTRUCT()
struct FLSSTCondition_IsBeyondLeashDistanceInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="LS/AI")
	bool bIsBeyondLeashDistance = false;

	UPROPERTY(EditAnywhere, Category="LS/AI")
	bool bInvert = false;
};

/** StateTree condition used for return-home transitions when the monster is beyond leash distance. */
USTRUCT(meta=(DisplayName="LS Is Beyond Leash Distance", Category="LS|AI"))
struct LOSTSIGNAL_API FLSSTCondition_IsBeyondLeashDistance : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLSSTCondition_IsBeyondLeashDistanceInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};
