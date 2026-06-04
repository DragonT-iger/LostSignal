#pragma once

#include "CoreMinimal.h"
#include "StateTreeConditionBase.h"
#include "LSSTCondition_IsAttacking.generated.h"

/** Input payload for checking the attack state mirrored by the monster evaluator. */
USTRUCT()
struct FLSSTCondition_IsAttackingInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="LS/AI")
	bool bIsAttacking = false;

	UPROPERTY(EditAnywhere, Category="LS/AI")
	bool bInvert = false;
};

/** StateTree condition used to block attack exits while a monster attack montage/ability is active. */
USTRUCT(meta=(DisplayName="LS Is Attacking", Category="LS|AI"))
struct LOSTSIGNAL_API FLSSTCondition_IsAttacking : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLSSTCondition_IsAttackingInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};
