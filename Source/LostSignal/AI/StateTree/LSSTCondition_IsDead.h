#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "LSSTCondition_IsDead.generated.h"

/** Input payload for checking the death state mirrored by the monster evaluator. */
USTRUCT()
struct FLSSTCondition_IsDeadInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="LS/AI")
	bool bIsDead = false;

	UPROPERTY(EditAnywhere, Category="LS/AI")
	bool bInvert = false;
};

/** StateTree condition used for transitions into or out of the terminal Dead state. */
USTRUCT(meta=(DisplayName="LS Is Dead", Category="LS|AI"))
struct LOSTSIGNAL_API FLSSTCondition_IsDead : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLSSTCondition_IsDeadInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};
