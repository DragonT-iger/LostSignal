#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "LSSTCondition_HasInterestLocation.generated.h"

/** Input payload for checking whether search/alert has a valid destination. */
USTRUCT()
struct FLSSTCondition_HasInterestLocationInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="LS/AI")
	bool bHasInterestLocation = false;

	UPROPERTY(EditAnywhere, Category="LS/AI")
	bool bInvert = false;
};

/** StateTree condition used for transitions that need a search destination. */
USTRUCT(meta=(DisplayName="LS Has Interest Location", Category="LS|AI"))
struct LOSTSIGNAL_API FLSSTCondition_HasInterestLocation : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLSSTCondition_HasInterestLocationInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};
