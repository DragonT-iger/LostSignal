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

	UFUNCTION(BlueprintPure, Category="AI|Sense")
	bool HasVisualTarget() const;

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
	void SetThreatMultiplier(float InThreatMultiplier);

	UFUNCTION(BlueprintCallable, Category="AI|Sense")
	void SetForceMaxSightRadius(bool bInForceMaxSightRadius);

	UFUNCTION(BlueprintCallable, Category="AI|Sense")
	void ClearVisualTarget();

	UFUNCTION(BlueprintCallable, Category="AI|Sense")
	void ClearInterest();

	bool CanSeeActor(const AActor* Actor) const;

private:
	void UpdateSensing(float DeltaTime);
	AActor* FindBestVisibleTarget() const;
	bool IsOwnerDead() const;
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

	UPROPERTY(EditAnywhere, Category="LS/AI|Sense", meta=(ClampMin="0.0"))
	float LeashDistance = 2000.0f;

	UPROPERTY(EditAnywhere, Category="LS/AI|Debug")
	bool bDrawSenseDebug = false;

	UPROPERTY(EditAnywhere, Category="LS/AI|Debug")
	bool bLogNoiseDebug = false;

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
	float ThreatMultiplier = 1.0f;

	UPROPERTY(VisibleAnywhere, Category="LS/AI|Sense")
	bool bForceMaxSightRadius = false;
};
