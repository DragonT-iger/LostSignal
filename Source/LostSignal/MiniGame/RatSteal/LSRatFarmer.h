#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MiniGame/RatSteal/LSRatTypes.h"
#include "LSRatFarmer.generated.h"

class UBoxComponent;
class UPaperFlipbook;
class UPaperFlipbookComponent;
class ULSRatYSortComponent;
class ALSRatAttackIndicator;
class ALSRatPlayer;

/**
 * 농부 AI (11_Entity_Farmer). enum 상태 머신 Patrol→Chase→Attack.
 * 원작 Alert는 빈 함수(미구현)라 미채택. 감지는 거리 검사로 처리:
 *  - Patrol/Chase 존(350/650)은 초기 위치 고정, Attack 존(200)은 농부 부착.
 * 플레이어가 Hide(부쉬)면 감지하지 않고 Patrol로 복귀.
 */
UCLASS()
class LOSTSIGNAL_API ALSRatFarmer : public APawn
{
	GENERATED_BODY()

public:
	ALSRatFarmer();

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "LS/RatSteal")
	ELSRatFarmerState GetFarmerState() const { return State; }

protected:
	virtual void BeginPlay() override;

	void DoPatrol(float DeltaSeconds);
	void DoChase(float DeltaSeconds);
	void DoAttack(float DeltaSeconds);
	void ChangeState(ELSRatFarmerState NewState);

	void CreateIndicators();
	void ExecuteAttack();
	void ClearIndicators();

	void MoveTowards(const FVector2D& TargetXZ, float DeltaSeconds);
	void FaceDirection(float DirX);
	void SetFlipbookSafe(UPaperFlipbook* Flipbook);

	ALSRatPlayer* GetRatPlayer() const;
	FVector2D GetXZ(const FVector& Location) const { return FVector2D(Location.X, Location.Z); }

	UPROPERTY(VisibleAnywhere, Category = "LS/RatSteal")
	TObjectPtr<UBoxComponent> CollisionBox;

	UPROPERTY(VisibleAnywhere, Category = "LS/RatSteal")
	TObjectPtr<UPaperFlipbookComponent> Sprite;

	UPROPERTY(VisibleAnywhere, Category = "LS/RatSteal")
	TObjectPtr<ULSRatYSortComponent> YSort;

	// ---- 밸런스 (50_Content_Balance, 원작 그대로) ----

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Balance")
	float MoveSpeed = 200.f;

	/** 순찰 반경 (초기 위치 고정) */
	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Balance")
	float PatrolRadius = 350.f;

	/** 추적 유지 반경 (초기 위치 고정, 이탈 시 순찰 복귀 leash) */
	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Balance")
	float ChaseRadius = 650.f;

	/** 공격 진입 반경 (농부 부착) */
	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Balance")
	float AttackRadius = 200.f;

	/** 순찰 목표 중심 가중 지수 (원작 biasExp=2 — 중심 쪽 가중) */
	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Balance")
	float PatrolBiasExp = 2.f;

	/** 지시자 표시 후 타격까지 (회피 여유) */
	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Balance")
	float AttackDelay = 1.5f;

	/** 지시자 재생성 주기 */
	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Balance")
	float AttackInterval = 0.5f;

	/** 공격 애니메이션 동안 행동 잠금 (원작은 attack 클립 길이 — 플레이로 조정) */
	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Balance")
	float AttackAnimDuration = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal")
	TSubclassOf<ALSRatAttackIndicator> IndicatorClass;

	// ---- 애니메이션 (idle/angryidle/walk/angrywalk/attack — 에셋 임포트 후 할당) ----

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Anim")
	TObjectPtr<UPaperFlipbook> IdleFlipbook;

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Anim")
	TObjectPtr<UPaperFlipbook> AngryIdleFlipbook;

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Anim")
	TObjectPtr<UPaperFlipbook> WalkFlipbook;

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Anim")
	TObjectPtr<UPaperFlipbook> AngryWalkFlipbook;

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Anim")
	TObjectPtr<UPaperFlipbook> AttackFlipbook;

private:
	ELSRatFarmerState State = ELSRatFarmerState::Patrol;

	FVector2D InitialPosition = FVector2D::ZeroVector;
	FVector2D PatrolTarget = FVector2D::ZeroVector;
	bool bHasPatrolTarget = false;

	float AttackTimer = 0.f;
	float AttackIntervalTimer = 0.f;
	float AttackAnimRemaining = 0.f;

	UPROPERTY()
	TArray<TObjectPtr<ALSRatAttackIndicator>> Indicators;
};
