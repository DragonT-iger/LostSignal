#pragma once

#include "CoreMinimal.h"
#include "Tasks/StateTreeAITask.h"
#include "LSSTTask_Patrol.generated.h"

class AAIController;
class ALSEnemyCharacter;

/** 순찰 진행 단계. */
UENUM()
enum class ELSPatrolPhase : uint8
{
	Walking,
	LookingAround
};

/** Input/runtime payload for HomeLocation-anchored wander patrol. */
USTRUCT()
struct FLSSTTask_PatrolInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="LS/AI")
	TObjectPtr<AAIController> AIController = nullptr;

	UPROPERTY(EditAnywhere, Category="LS/AI")
	TObjectPtr<ALSEnemyCharacter> EnemyCharacter = nullptr;

	/** 순찰 기준점. evaluator의 HomeLocation에 바인딩(미바인딩 시 진입 위치로 폴백). */
	UPROPERTY(EditAnywhere, Category="LS/AI")
	FVector HomeLocation = FVector::ZeroVector;

	/** 1구간 직선 이동 거리. */
	UPROPERTY(EditAnywhere, Category="LS/AI", meta=(ClampMin="0.0"))
	float WalkDistance = 500.0f;

	/** HomeLocation 기준 최대 배회 반경(이탈 한계). */
	UPROPERTY(EditAnywhere, Category="LS/AI", meta=(ClampMin="0.0"))
	float PatrolRadius = 1000.0f;

	/** 네비 경계(충돌체)로부터 멈출 여유거리. */
	UPROPERTY(EditAnywhere, Category="LS/AI", meta=(ClampMin="0.0"))
	float ObstacleStopMargin = 100.0f;

	/** 둘러보기(정지 대기) 시간. */
	UPROPERTY(EditAnywhere, Category="LS/AI", meta=(ClampMin="0.0"))
	float LookAroundDuration = 2.0f;

	/** 도착 판정 반경. */
	UPROPERTY(EditAnywhere, Category="LS/AI", meta=(ClampMin="0.0"))
	float AcceptanceRadius = 50.0f;

	/** 반경 초과 시 Home 방향 기준 좌우 무작위 분산각. */
	UPROPERTY(EditAnywhere, Category="LS/AI", meta=(ClampMin="0.0"))
	float HomeBiasSpreadDegrees = 60.0f;

	/** 순찰 이동 속도 배수(기본 MaxWalkSpeed × 배수). */
	UPROPERTY(EditAnywhere, Category="LS/AI", meta=(ClampMin="0.0"))
	float PatrolSpeedMultiplier = 0.5f;

	UPROPERTY(Transient, VisibleAnywhere, Category="LS/AI")
	ELSPatrolPhase Phase = ELSPatrolPhase::Walking;

	UPROPERTY(Transient, VisibleAnywhere, Category="LS/AI")
	float LookAroundElapsed = 0.0f;
};

/** StateTree task: HomeLocation 주변을 직선 구간 이동 + 정지 대기로 배회하는 순찰. */
USTRUCT(meta=(DisplayName="LS Patrol", Category="LS|AI|Action"))
struct LOSTSIGNAL_API FLSSTTask_Patrol : public FStateTreeAIActionTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FLSSTTask_PatrolInstanceData;

	FLSSTTask_Patrol();

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

private:
	/** 새 직선 구간 목적지를 산출하고 MoveTo를 발행한다(Phase=Walking). */
	void BeginWalkSegment(FInstanceDataType& InstanceData) const;
};
