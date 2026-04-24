#pragma once

#include "CoreMinimal.h"
#include "StateTreeEvaluatorBase.h"
#include "LSSTEvaluator_MonsterSense.generated.h"

class AAIController;
class ALSEnemyCharacter;
class ULSMonsterCombatComponent;
class ULSMonsterSenseComponent;

/** Bindable runtime snapshot filled from monster sense/combat components. */
USTRUCT()
struct FLSSTEvaluator_MonsterSenseInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Context")
	TObjectPtr<AAIController> AIController = nullptr;

	UPROPERTY(EditAnywhere, Category="Context")
	TObjectPtr<ALSEnemyCharacter> EnemyCharacter = nullptr;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Internal")
	TObjectPtr<ULSMonsterSenseComponent> SenseComponent = nullptr;

	UPROPERTY(Transient, VisibleAnywhere, Category = "Internal")
	TObjectPtr<ULSMonsterCombatComponent> CombatComponent = nullptr;

	UPROPERTY(EditAnywhere, Category="Output")
	TObjectPtr<AActor> CurrentTarget = nullptr;

	UPROPERTY(EditAnywhere, Category="Output")
	FVector InterestLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category="Output")
	FVector HomeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category="Output")
	float DistanceToTarget = 0.0f;

	UPROPERTY(EditAnywhere, Category="Output")
	float LeashDistance = 0.0f;

	UPROPERTY(EditAnywhere, Category="Output")
	float AlertDuration = 0.0f;

	UPROPERTY(EditAnywhere, Category="Output")
	float AlertMoveSpeedMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, Category="Output")
	bool bHasVisualTarget = false;

	UPROPERTY(EditAnywhere, Category="Output")
	bool bHasInterestLocation = false;
};

/** StateTree evaluator that exposes monster sensing data to transitions and tasks. */
USTRUCT(meta=(DisplayName="LS Monster Sense Evaluator", Category="LS|AI"))
struct LOSTSIGNAL_API FLSSTEvaluator_MonsterSense : public FStateTreeEvaluatorCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLSSTEvaluator_MonsterSenseInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual void TreeStart(FStateTreeExecutionContext& Context) const override;
	virtual void Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

private:
	void UpdateData(FStateTreeExecutionContext& Context) const;
};
