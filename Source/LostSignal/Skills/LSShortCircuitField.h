#pragma once

#include "CoreMinimal.h"
#include "Combat/LSCombatTypes.h"
#include "GameFramework/Actor.h"
#include "LSShortCircuitField.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class ULSShortCircuitSkillDataAsset;
class ULSSkillDataAsset;

UCLASS()
class LOSTSIGNAL_API ALSShortCircuitField : public AActor
{
	GENERATED_BODY()

public:
	ALSShortCircuitField();

	void InitializeField(AActor* InSourceActor, ULSShortCircuitSkillDataAsset* InSkillData);
	bool ExplodeByExecution(
		AActor* InstigatorActor,
		const ULSSkillDataAsset* ExecutionSkillData,
		float AttackCoefficient,
		bool bCanCrit,
		ELSBreakPowerTier BreakPower,
		float RadiusOverride = 0.0f,
		bool bDestroyAfterExplosion = true);

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Skill|ShortCircuit")
	TObjectPtr<USphereComponent> AreaComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Skill|ShortCircuit|Debug")
	TObjectPtr<UStaticMeshComponent> DebugFieldSphereMeshComponent;

	UPROPERTY(Transient, VisibleInstanceOnly, Category="LS/Skill|ShortCircuit")
	TObjectPtr<AActor> SourceActor;

	UPROPERTY(Transient, VisibleInstanceOnly, Category="LS/Skill|ShortCircuit")
	TObjectPtr<ULSShortCircuitSkillDataAsset> SkillData;

private:
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastBlinkDebugFieldMesh(float Radius, float VisibleSeconds);

	FTimerHandle PulseTimerHandle;
	FTimerHandle DebugFieldMeshBlinkTimerHandle;
	int32 PulsesRemaining = 0;
	bool bFieldStarted = false;

	UPROPERTY(ReplicatedUsing=OnRep_DebugFieldMeshRadius, Transient, VisibleInstanceOnly, Category="LS/Skill|ShortCircuit|Debug", meta=(AllowPrivateAccess="true"))
	float DebugFieldMeshRadius = 0.0f;

	UFUNCTION()
	void OnRep_DebugFieldMeshRadius();

	void StartField();
	void ConfigureFromSkillData();
	void ApplyPulse();
	void ApplySlowEffect(AActor* TargetActor) const;
	void SetDebugFieldMeshVisible(bool bVisible, float Radius);
	void HideDebugFieldMesh();
};
