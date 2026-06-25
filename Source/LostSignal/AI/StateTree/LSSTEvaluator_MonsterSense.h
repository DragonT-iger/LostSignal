#pragma once

#include "CoreMinimal.h"
#include "StateTreeEvaluatorBase.h"
#include "LSSTEvaluator_MonsterSense.generated.h"

class AAIController;
class ALSEnemyCharacter;
class UAbilitySystemComponent;
class ULSMonsterCombatComponent;
class ULSMonsterSenseComponent;

/** Bindable runtime snapshot filled from monster sense/combat components. */
USTRUCT()
struct FLSSTEvaluator_MonsterSenseInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="LS/AI")
	TObjectPtr<AAIController> AIController = nullptr;

	UPROPERTY(EditAnywhere, Category="LS/AI")
	TObjectPtr<ALSEnemyCharacter> EnemyCharacter = nullptr;

	UPROPERTY(Transient, VisibleAnywhere, Category = "LS/AI")
	TObjectPtr<ULSMonsterSenseComponent> SenseComponent = nullptr;

	UPROPERTY(Transient, VisibleAnywhere, Category = "LS/AI")
	TObjectPtr<ULSMonsterCombatComponent> CombatComponent = nullptr;

	UPROPERTY(EditAnywhere, Category="LS/AI")
	TObjectPtr<AActor> CurrentTarget = nullptr;

	UPROPERTY(EditAnywhere, Category="LS/AI")
	FVector InterestLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category="LS/AI")
	FVector HomeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category="LS/AI")
	float DistanceToTarget = 0.0f;

	UPROPERTY(EditAnywhere, Category="LS/AI")
	float LeashDistance = 0.0f;

	UPROPERTY(EditAnywhere, Category="LS/AI")
	float DistanceFromHome = 0.0f;

	UPROPERTY(EditAnywhere, Category="LS/AI")
	float AlertDuration = 0.0f;

	UPROPERTY(EditAnywhere, Category="LS/AI")
	float AlertMoveSpeedMultiplier = 1.0f;

	/** 현재 타겟을 이번 틱 실제로 보고 있는지(FOV+LOS). Attack/즉시 추격 판정용. */
	UPROPERTY(EditAnywhere, Category="LS/AI")
	bool bHasVisualTarget = false;

	/** 타겟 보유 여부(시야 상실 후 기억 시간 포함). Chase 유지 판정용. */
	UPROPERTY(EditAnywhere, Category="LS/AI")
	bool bHasTarget = false;

	UPROPERTY(EditAnywhere, Category="LS/AI")
	bool bHasInterestLocation = false;

	/** 현재 거리에 발동 가능한(사거리 적합 + 쿨다운 준비) 액션이 있는지. Attack 진입 판정용. */
	UPROPERTY(EditAnywhere, Category="LS/AI")
	bool bHasUsableAction = false;

	UPROPERTY(EditAnywhere, Category="LS/AI")
	bool bIsBeyondLeashDistance = false;

	UPROPERTY(EditAnywhere, Category="LS/AI")
	bool bIsAttacking = false;

	/** Mirrors the monster ASC dead tag so every state can transition into Dead with one shared bool. */
	UPROPERTY(EditAnywhere, Category="LS/AI")
	bool bIsDead = false;

	/** Mirrors temporary CC movement lock state such as Override knockback. */
	UPROPERTY(EditAnywhere, Category="LS/AI")
	bool bIsKnockback = false;
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
