#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Tasks/StateTreeAITask.h"
#include "LSSTTask_WaitForKnockbackEnd.generated.h"

class AAIController;
class ALSEnemyCharacter;
class ULSMonsterCombatComponent;

/** Input payload for a temporary StateTree state that owns knockback interruption. */
USTRUCT()
struct FLSSTTask_WaitForKnockbackEndInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="LS/AI")
	TObjectPtr<AAIController> AIController = nullptr;

	UPROPERTY(EditAnywhere, Category="LS/AI")
	TObjectPtr<ALSEnemyCharacter> EnemyCharacter = nullptr;

	UPROPERTY(EditAnywhere, Category="LS/AI")
	TObjectPtr<ULSMonsterCombatComponent> CombatComponent = nullptr;

	UPROPERTY(EditAnywhere, Category="LS/AI")
	FGameplayTag AttackAbilityTag;

	UPROPERTY(EditAnywhere, Category="LS/AI")
	bool bFallbackToDefaultAttackTag = true;

	UPROPERTY(EditAnywhere, Category="LS/AI")
	bool bIsKnockback = false;
};

/** StateTree task that waits while LS.State.Knockback is active and lets Combat restart cleanly after it ends. */
USTRUCT(meta=(DisplayName="LS Wait For Knockback End", Category="LS|AI|Action"))
struct LOSTSIGNAL_API FLSSTTask_WaitForKnockbackEnd : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLSSTTask_WaitForKnockbackEndInstanceData;

	FLSSTTask_WaitForKnockbackEnd();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};
