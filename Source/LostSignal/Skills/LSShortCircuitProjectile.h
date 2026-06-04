#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LSShortCircuitProjectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class ULSShortCircuitSkillDataAsset;

UCLASS()
class LOSTSIGNAL_API ALSShortCircuitProjectile : public AActor
{
	GENERATED_BODY()

public:
	ALSShortCircuitProjectile();

	void InitializeProjectile(
		AActor* InSourceActor,
		ULSShortCircuitSkillDataAsset* InSkillData,
		const FVector& TargetLocation,
		float ProjectileDuration,
		float ProjectileArcHeight,
		float ProjectileLifeSeconds);

protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Skill|ShortCircuit")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Skill|ShortCircuit|Debug")
	TObjectPtr<UStaticMeshComponent> DebugProjectileMeshComponent;

	UPROPERTY(Transient, VisibleInstanceOnly, Category="LS/Skill|ShortCircuit")
	TObjectPtr<AActor> SourceActor;

	UPROPERTY(Transient, VisibleInstanceOnly, Category="LS/Skill|ShortCircuit")
	TObjectPtr<ULSShortCircuitSkillDataAsset> SkillData;

	UPROPERTY(Transient, VisibleInstanceOnly, Category="LS/Skill|ShortCircuit")
	FVector ImpactTargetLocation = FVector::ZeroVector;

	UPROPERTY(Transient, VisibleInstanceOnly, Category="LS/Skill|ShortCircuit")
	float RuntimeProjectileArcHeight = 0.0f;

private:
	UPROPERTY(Transient, VisibleInstanceOnly, Category="LS/Skill|ShortCircuit", meta=(AllowPrivateAccess="true"))
	FVector MovementStartLocation = FVector::ZeroVector;

	UPROPERTY(Transient, VisibleInstanceOnly, Category="LS/Skill|ShortCircuit", meta=(AllowPrivateAccess="true"))
	FVector MovementVisualTargetLocation = FVector::ZeroVector;

	UPROPERTY(Transient, VisibleInstanceOnly, Category="LS/Skill|ShortCircuit", meta=(AllowPrivateAccess="true"))
	float MovementElapsedSeconds = 0.0f;

	UPROPERTY(Transient, VisibleInstanceOnly, Category="LS/Skill|ShortCircuit", meta=(AllowPrivateAccess="true"))
	float MovementDurationSeconds = 0.0f;

	UPROPERTY(ReplicatedUsing=OnRep_DebugProjectileMesh, Transient, VisibleInstanceOnly, Category="LS/Skill|ShortCircuit|Debug", meta=(AllowPrivateAccess="true"))
	bool bShowDebugProjectileMesh = false;

	UFUNCTION()
	void OnRep_DebugProjectileMesh();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastDrawDebugImpact(FVector_NetQuantize ImpactLocation, FColor Color, float Duration);

	void FinishProjectile();
	void SpawnFieldAtLocation(const FVector& FieldLocation);
	void SetDebugProjectileMeshVisible(bool bVisible);
	void DrawDebugImpact(const FVector& ImpactLocation, const FColor& Color, float Duration) const;
};
