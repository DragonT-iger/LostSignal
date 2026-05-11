#pragma once

#include "CoreMinimal.h"
#include "Skills/LSSkill.h"
#include "LSShortCircuitSkill.generated.h"

class ALSShortCircuitField;
class ALSShortCircuitProjectile;

UCLASS(Blueprintable, BlueprintType)
class LOSTSIGNAL_API ULSShortCircuitSkill : public ULSSkill
{
	GENERATED_BODY()

public:
	ULSShortCircuitSkill();

	virtual bool ActivateSkill_Implementation(const FLSSkillActivationContext& Context) override;

	TSubclassOf<ALSShortCircuitProjectile> ResolveProjectileClass() const;
	TSubclassOf<ALSShortCircuitField> ResolveFieldClass() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|ShortCircuit")
	TSubclassOf<ALSShortCircuitProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|ShortCircuit")
	TSubclassOf<ALSShortCircuitField> FieldClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|ShortCircuit", meta=(ClampMin="0.0"))
	float ProjectileSpeed = 1200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|ShortCircuit", meta=(ClampMin="0.0"))
	float ProjectileArcHeight = 220.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|ShortCircuit", meta=(ClampMin="0.0"))
	float ProjectileLifeSeconds = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|ShortCircuit", meta=(ClampMin="0.0"))
	float ProjectileSpawnForwardOffset = 80.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|ShortCircuit")
	float ProjectileSpawnZOffset = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|ShortCircuit", meta=(ClampMin="0.0"))
	float FieldDuration = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|ShortCircuit", meta=(ClampMin="0.01"))
	float FieldPulseInterval = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|ShortCircuit|Debug")
	bool bEnableDebugVisualization = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|ShortCircuit|Debug", meta=(ClampMin="0.0"))
	float DebugDrawDuration = 3.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|ShortCircuit|Debug")
	FColor DebugProjectileColor = FColor::Cyan;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|ShortCircuit|Debug")
	FColor DebugFieldColor = FColor::Green;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|ShortCircuit|Debug")
	FColor DebugPulseColor = FColor::Yellow;
};
