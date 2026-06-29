#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "LSSTCondition_IsDormantByDistance.generated.h"

USTRUCT()
struct FLSSTCondition_IsDormantByDistanceInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="LS/AI")
	bool bIsDormantByDistance = false;

	UPROPERTY(EditAnywhere, Category="LS/AI")
	bool bInvert = false;
};

USTRUCT(meta=(DisplayName="LS Is Dormant By Distance", Category="LS|AI"))
struct LOSTSIGNAL_API FLSSTCondition_IsDormantByDistance : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLSSTCondition_IsDormantByDistanceInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};
