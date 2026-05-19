#pragma once

#include "CoreMinimal.h"
#include "Combat/LSCombatTypes.h"
#include "Skills/LSSkillDataAsset.h"
#include "LSBypassSkillDataAsset.generated.h"

class ALSBypassHologramActor;
class UGameplayEffect;
class UMaterialInterface;

/** Bypass-specific data. Macro variants can reserve the next basic attack combo index here. */
UCLASS(BlueprintType)
class LOSTSIGNAL_API ULSBypassSkillDataAsset : public ULSSkillDataAsset
{
	GENERATED_BODY()

public:
	ULSBypassSkillDataAsset();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Bypass", meta=(ClampMin="0.0"))
	float FallbackDistance = 600.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Bypass", meta=(ClampMin="0.0"))
	float FallbackDuration = 0.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Bypass|Macro")
	bool bSetComboIndexOverrideOnFinish = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Bypass|Macro", meta=(ClampMin="0"))
	int32 ComboIndexOverride = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Bypass|Macro", meta=(ClampMin="0.0"))
	float ComboIndexOverrideWindowSeconds = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Bypass|Spoofing")
	bool bSpawnHologramOnStart = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Bypass|Spoofing")
	TSubclassOf<ALSBypassHologramActor> HologramActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Bypass|Spoofing")
	TObjectPtr<UMaterialInterface> HologramMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Bypass|Spoofing", meta=(ClampMin="0.0"))
	float HologramLifeSeconds = 1.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Bypass|Spoofing")
	bool bPullTargetsToHologram = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Bypass|Spoofing", meta=(ClampMin="0.0"))
	float PullRadius = 450.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Bypass|Spoofing", meta=(ClampMin="0.0"))
	float PullSpeed = 900.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Bypass|Spoofing", meta=(ClampMin="0.0"))
	float PullDuration = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Bypass|Spoofing")
	float PullUpSpeed = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Bypass|Spoofing")
	ELSBreakPowerTier PullBreakPower = ELSBreakPowerTier::SpecialAttack;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Bypass|Spoofing")
	TSubclassOf<UGameplayEffect> StunEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Bypass|Spoofing", meta=(ClampMin="0.0"))
	float StunDuration = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Bypass|Debug")
	bool bEnableSpoofingDebugLog = false;
};
