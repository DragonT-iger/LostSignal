#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LSMonsterSenseComponent.generated.h"

struct FLSMonsterArchetypeRow;
struct FLSNoiseEvent;

/** Server-side sight/hearing cache used by StateTree decisions. */
UCLASS(ClassGroup=(LS), meta=(BlueprintSpawnableComponent))
class LOSTSIGNAL_API ULSMonsterSenseComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSMonsterSenseComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void ApplyArchetype(const FLSMonsterArchetypeRow& Row);

	void RegisterNoiseEvent(const FLSNoiseEvent& NoiseEvent);

	UFUNCTION(BlueprintCallable, Category="AI|Sense")
	void SetCurrentTargetFromDamage(AActor* DamageInstigator);

	UFUNCTION(BlueprintPure, Category="AI|Sense")
	AActor* GetCurrentTarget() const { return CurrentTarget.Get(); }

	/** True when the current target is actually seen this tick (FOV + LOS). */
	UFUNCTION(BlueprintPure, Category="AI|Sense")
	bool HasVisualTarget() const;

	/** True while a target is held, including the post-sight memory window. */
	UFUNCTION(BlueprintPure, Category="AI|Sense")
	bool HasTarget() const;

	UFUNCTION(BlueprintPure, Category="AI|Sense")
	bool HasInterestLocation() const;

	UFUNCTION(BlueprintPure, Category="AI|Sense")
	FVector GetInterestLocation() const;

	UFUNCTION(BlueprintPure, Category="AI|Sense")
	FVector GetHomeLocation() const { return HomeLocation; }

	UFUNCTION(BlueprintPure, Category="AI|Sense")
	float GetAlertDuration() const { return AlertDuration; }

	UFUNCTION(BlueprintPure, Category="AI|Sense")
	float GetLeashDistance() const { return LeashDistance; }

	UFUNCTION(BlueprintPure, Category="AI|Sense")
	float GetDistanceFromHome() const;

	UFUNCTION(BlueprintPure, Category="AI|Sense")
	bool IsBeyondLeashDistance() const;

	UFUNCTION(BlueprintPure, Category="AI|Sense")
	float GetAlertMoveSpeedMultiplier() const { return AlertMoveSpeedMultiplier; }

	UFUNCTION(BlueprintPure, Category="AI|Sense")
	float GetCurrentSightRadius() const;

	UFUNCTION(BlueprintCallable, Category="AI|Sense")
	void SetForceMaxSightRadius(bool bInForceMaxSightRadius);

	UFUNCTION(BlueprintCallable, Category="LS/AI|Sense")
	void SetReturnHomeMode(bool bInReturnHomeMode);

	UFUNCTION(BlueprintCallable, Category="AI|Sense")
	void ClearVisualTarget();

	UFUNCTION(BlueprintCallable, Category="AI|Sense")
	void ClearInterest();

	bool CanSeeActor(const AActor* Actor) const;

private:
	void UpdateSensing(float DeltaTime);
	AActor* FindBestVisibleTarget() const;
	/** Commits a new target; captures the aggro anchor only on the first acquisition. */
	void SetTarget(AActor* NewTarget, bool bCaptureAnchor);
	/** Fully drops the current target, anchor and memory timer (keeps InterestLocation). */
	void ReleaseTarget();
	bool IsLocationBeyondLeashDistance(const FVector& Location) const;
	bool ShouldSuppressReturnHomeInterest(const FVector& InterestCandidateLocation) const;
	bool IsOwnerDead() const;
	bool IsOwnerAttacking() const;
	void DrawSenseDebug() const;

	UPROPERTY(EditAnywhere, Category="LS/AI|Sense", meta=(ClampMin="0.0"))
	float BaseSightRadius = 1200.0f;

	UPROPERTY(EditAnywhere, Category="LS/AI|Sense", meta=(ClampMin="0.0"))
	float MaxSightRadius = 1800.0f;

	UPROPERTY(EditAnywhere, Category="LS/AI|Sense", meta=(ClampMin="0.0", ClampMax="180.0"))
	float SightHalfAngleDegrees = 55.0f;

	UPROPERTY(EditAnywhere, Category="LS/AI|Sense", meta=(ClampMin="0.0"))
	float HearingRadius = 900.0f;

	UPROPERTY(EditAnywhere, Category="LS/AI|Sense", meta=(ClampMin="0.0"))
	float AlertDuration = 5.0f;

	UPROPERTY(EditAnywhere, Category="LS/AI|Sense", meta=(ClampMin="0.0"))
	float AlertMoveSpeedMultiplier = 1.2f;

	// 최초 인식 위치(앵커)에서 이 거리를 벗어나면 타겟 해제(P0). 기획 30m 기준.
	UPROPERTY(EditAnywhere, Category="LS/AI|Sense", meta=(ClampMin="0.0"))
	float LeashDistance = 3000.0f;

	// 시야에서 타겟을 놓친 뒤 타겟을 유지하는 시간(P3). 경과 시 타겟 해제.
	UPROPERTY(EditAnywhere, Category="LS/AI|Sense", meta=(ClampMin="0.0"))
	float LostSightMemorySeconds = 5.0f;

	UPROPERTY(EditAnywhere, Category="LS/AI|Debug")
	bool bDrawSenseDebug = false;

	UPROPERTY(EditAnywhere, Category="LS/AI|Debug", meta=(ClampMin="0.0"))
	float SenseDebugDrawHeight = 35.0f;

	UPROPERTY(VisibleAnywhere, Category="LS/AI|Sense")
	TWeakObjectPtr<AActor> CurrentTarget;

	UPROPERTY(VisibleAnywhere, Category="LS/AI|Sense")
	FVector InterestLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, Category="LS/AI|Sense")
	FVector HomeLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, Category="LS/AI|Sense")
	bool bHasInterestLocation = false;

	UPROPERTY(VisibleAnywhere, Category="LS/AI|Sense")
	bool bForceMaxSightRadius = false;

	UPROPERTY(Transient, VisibleAnywhere, Category="LS/AI|Sense")
	bool bReturnHomeMode = false;

	// 최초 인식 시점의 몬스터 위치(P0 이탈 판정 기준점). 복귀 목적지(HomeLocation)와 별개.
	UPROPERTY(Transient, VisibleAnywhere, Category="LS/AI|Sense")
	FVector AggroAnchorLocation = FVector::ZeroVector;

	UPROPERTY(Transient, VisibleAnywhere, Category="LS/AI|Sense")
	bool bHasAggroAnchor = false;

	// 시야에서 타겟을 놓친 뒤 경과 시간(P3 기억 타이머).
	UPROPERTY(Transient, VisibleAnywhere, Category="LS/AI|Sense")
	float TimeSinceTargetLastSeen = 0.0f;

	// 이번 틱에 현재 타겟을 실제로 봤는지.
	UPROPERTY(Transient, VisibleAnywhere, Category="LS/AI|Sense")
	bool bTargetVisibleThisTick = false;
};
