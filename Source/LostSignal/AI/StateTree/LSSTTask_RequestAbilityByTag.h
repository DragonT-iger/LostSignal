#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Tasks/StateTreeAITask.h"
#include "LSSTTask_RequestAbilityByTag.generated.h"

class ALSEnemyCharacter;
class ULSMonsterCombatComponent;

/** Input payload for requesting a monster GAS ability from StateTree. */
USTRUCT()
struct FLSSTTask_RequestAbilityByTagInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="LS/AI")
	TObjectPtr<ALSEnemyCharacter> EnemyCharacter = nullptr;

	UPROPERTY(EditAnywhere, Category="LS/AI")
	TObjectPtr<ULSMonsterCombatComponent> CombatComponent = nullptr;

	UPROPERTY(EditAnywhere, Category="LS/AI")
	FGameplayTag AbilityTag;

	UPROPERTY(EditAnywhere, Category="LS/AI")
	bool bFallbackToDefaultAttackTag = true;

	UPROPERTY(EditAnywhere, Category="LS/AI")
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(EditAnywhere, Category="LS/AI", meta=(ClampMin="0.0"))
	float CancelIfTargetFartherThan = 180.0f;

	UPROPERTY(EditAnywhere, Category="LS/AI")
	bool bCancelAbilityWhenTargetLeavesRange = true;

	UPROPERTY(Transient, VisibleAnywhere, Category="LS/AI")
	FGameplayTag ActiveAbilityTag;
};

/** StateTree task that triggers a monster ability by gameplay tag. */
USTRUCT(meta=(DisplayName="LS Request Ability By Tag", Category="LS|AI|Action"))
struct LOSTSIGNAL_API FLSSTTask_RequestAbilityByTag : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLSSTTask_RequestAbilityByTagInstanceData;

	FLSSTTask_RequestAbilityByTag();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};
