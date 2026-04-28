#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "LSSTCondition_HasVisualTarget.generated.h"

/** Input payload for checking whether the monster currently sees a target. */
USTRUCT()
struct FLSSTCondition_HasVisualTargetInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="LS/AI")
	bool bHasVisualTarget = false;

	UPROPERTY(EditAnywhere, Category="LS/AI")
	bool bInvert = false;
};

/** StateTree condition used for transitions gated by live visual contact. */
USTRUCT(meta=(DisplayName="LS Has Visual Target", Category="LS|AI"))
struct LOSTSIGNAL_API FLSSTCondition_HasVisualTarget : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLSSTCondition_HasVisualTargetInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};
