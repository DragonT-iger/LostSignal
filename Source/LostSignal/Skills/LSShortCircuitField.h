#pragma once

#include "CoreMinimal.h"
#include "Combat/LSCombatTypes.h"
#include "GameFramework/Actor.h"
#include "LSShortCircuitField.generated.h"

class USphereComponent;
class UNiagaraComponent;
class UNiagaraSystem;
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Skill|ShortCircuit|VFX")
	TObjectPtr<UNiagaraComponent> FieldNiagaraComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|ShortCircuit|VFX")
	TObjectPtr<UNiagaraSystem> PulseNiagaraSystem;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|ShortCircuit|VFX")
	TObjectPtr<UNiagaraSystem> ExplosionNiagaraSystem;

	UPROPERTY(Transient, VisibleInstanceOnly, Category="LS/Skill|ShortCircuit")
	TObjectPtr<AActor> SourceActor;

	UPROPERTY(Transient, VisibleInstanceOnly, Category="LS/Skill|ShortCircuit")
	TObjectPtr<ULSShortCircuitSkillDataAsset> SkillData;

private:
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayPulseEffect(float Radius);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayExplosionEffect(FVector_NetQuantize EffectLocation, float Radius);

	FTimerHandle PulseTimerHandle;
	int32 PulsesRemaining = 0;
	bool bFieldStarted = false;

	UPROPERTY(ReplicatedUsing=OnRep_FieldVisualParams, Transient, VisibleInstanceOnly, Category="LS/Skill|ShortCircuit", meta=(AllowPrivateAccess="true"))
	float FieldRadius = 0.0f;

	UPROPERTY(ReplicatedUsing=OnRep_FieldVisualParams, Transient, VisibleInstanceOnly, Category="LS/Skill|ShortCircuit", meta=(AllowPrivateAccess="true"))
	float FieldDurationSeconds = 0.0f;

	UPROPERTY(ReplicatedUsing=OnRep_FieldVisualParams, Transient, VisibleInstanceOnly, Category="LS/Skill|ShortCircuit", meta=(AllowPrivateAccess="true"))
	float FieldPulseIntervalSeconds = 0.0f;

	UFUNCTION()
	void OnRep_FieldVisualParams();

	void StartField();
	void ConfigureFromSkillData();
	void ApplyPulse();
	void ApplySlowEffect(AActor* TargetActor) const;
	void ApplyFieldRadius(float Radius);
	void ApplyFieldNiagaraParameters();
};
