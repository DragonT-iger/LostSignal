#pragma once

#include "CoreMinimal.h"
#include "Tasks/StateTreeAITask.h"
#include "LSSTTask_ClearInterest.generated.h"

class ALSEnemyCharacter;
class ULSMonsterSenseComponent;

/** Input payload for clearing the monster's current investigation destination. */
USTRUCT()
struct FLSSTTask_ClearInterestInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="LS/AI")
	TObjectPtr<ALSEnemyCharacter> EnemyCharacter = nullptr;

	UPROPERTY(EditAnywhere, Category="LS/AI")
	TObjectPtr<ULSMonsterSenseComponent> SenseComponent = nullptr;
};

/** StateTree task that clears InterestLocation after an investigate/search state finishes. */
USTRUCT(meta=(DisplayName="LS Clear Interest", Category="LS|AI|Action"))
struct LOSTSIGNAL_API FLSSTTask_ClearInterest : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLSSTTask_ClearInterestInstanceData;

	FLSSTTask_ClearInterest();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
