#pragma once

#include "Abilities/GameplayAbility.h"
#include "Combat/LSCombatTypes.h"
#include "LSGA_Execution.generated.h"

class UGameplayEffect;
class ULSExecutionSkillDataAsset;

/** Ultimate skill: dash slash, sheath damage, and Short Circuit field detonation. */
UCLASS()
class LOSTSIGNAL_API ULSGA_Execution : public UGameplayAbility
{
	GENERATED_BODY()

public:
	ULSGA_Execution();

	bool ResolveMovementParams(const class ULSSkillDataAsset* SkillData, float& OutDistance, float& OutDuration) const;

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	UPROPERTY(EditDefaultsOnly, Category="LS/Skill|Execution")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, Category="LS/Skill|Execution", meta=(ClampMin="0.0"))
	float FallbackAttackCoefficient = 3.0f;

	UPROPERTY(EditDefaultsOnly, Category="LS/Skill|Execution")
	ELSBreakPowerTier FallbackBreakPower = ELSBreakPowerTier::HardCrowdControl;

private:
	void PerformSheathHit();
	int32 ConsumeCombatAccelerationStacks(AActor* SourceActor) const;
	bool IsPointInExecutionArea(const FVector& Point) const;
	void IgnoreEnemiesForDash(ACharacter* SourceCharacter);
	void ClearIgnoredEnemiesForDash(ACharacter* SourceCharacter);

	FTimerHandle SheathHitTimerHandle;
	uint16 RootMotionSourceID = 0;
	TArray<TWeakObjectPtr<AActor>> IgnoredEnemyActors;

	UPROPERTY(Transient)
	TObjectPtr<ULSExecutionSkillDataAsset> ExecutionData = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<class ULSSkillDataAsset> SkillData = nullptr;

	FVector CachedStartLocation = FVector::ZeroVector;
	FVector CachedDirection = FVector::ForwardVector;
	float CachedDashDistance = 0.0f;
	float CachedSlashWidth = 0.0f;
	float CachedAttackCoefficient = 0.0f;
	int32 CachedConsumedAccelerationStacks = 0;
};
