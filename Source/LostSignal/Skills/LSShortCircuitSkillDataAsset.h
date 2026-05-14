#pragma once

#include "CoreMinimal.h"
#include "Skills/LSSkillDataAsset.h"
#include "LSShortCircuitSkillDataAsset.generated.h"

class ALSShortCircuitField;
class ALSShortCircuitProjectile;

/**
 * Short Circuit-specific data.
 * GameplayAbility owns execution; projectile/field actors read only this data asset.
 */
UCLASS(BlueprintType)
class LOSTSIGNAL_API ULSShortCircuitSkillDataAsset : public ULSSkillDataAsset
{
	GENERATED_BODY()

public:
	ULSShortCircuitSkillDataAsset();

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
