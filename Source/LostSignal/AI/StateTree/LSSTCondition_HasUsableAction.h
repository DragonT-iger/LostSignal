#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "LSSTCondition_HasUsableAction.generated.h"

/** Input payload for checking whether the monster has an action usable at the current target distance. */
USTRUCT()
struct FLSSTCondition_HasUsableActionInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="LS/AI")
	bool bHasUsableAction = false;

	UPROPERTY(EditAnywhere, Category="LS/AI")
	bool bInvert = false;
};

/** StateTree condition used to gate Attack entry on a distance/cooldown-eligible monster action. */
USTRUCT(meta=(DisplayName="LS Has Usable Action", Category="LS|AI"))
struct LOSTSIGNAL_API FLSSTCondition_HasUsableAction : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLSSTCondition_HasUsableActionInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};
