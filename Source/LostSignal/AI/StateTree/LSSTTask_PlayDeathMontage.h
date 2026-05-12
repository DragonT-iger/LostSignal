#pragma once

#include "CoreMinimal.h"
#include "Tasks/StateTreeAITask.h"
#include "LSSTTask_PlayDeathMontage.generated.h"

class ALSEnemyCharacter;

/** Input payload for playing the enemy death montage from the Dead StateTree state. */
USTRUCT()
struct FLSSTTask_PlayDeathMontageInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="LS/AI")
	TObjectPtr<ALSEnemyCharacter> EnemyCharacter = nullptr;
};

/** StateTree task that plays the enemy-authored death montage before terminal AI shutdown. */
USTRUCT(meta=(DisplayName="LS Play Death Montage", Category="LS|AI|Action"))
struct LOSTSIGNAL_API FLSSTTask_PlayDeathMontage : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLSSTTask_PlayDeathMontageInstanceData;

	FLSSTTask_PlayDeathMontage();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};
