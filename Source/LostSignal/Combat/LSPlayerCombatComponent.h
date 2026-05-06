#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LSPlayerCombatComponent.generated.h"

class UAnimMontage;
class UGameplayAbility;
class UGameplayEffect;
class ULSAimComponent;
class ULSCharacterCombatComponent;

UCLASS(ClassGroup=(LS), meta=(BlueprintSpawnableComponent))
class LOSTSIGNAL_API ULSPlayerCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSPlayerCombatComponent();

	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	bool RequestBasicAttack();

	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	bool RequestDash();

	bool RequestDash(const FVector& DashDirection);
	bool PredictDashMovement(const FVector& DashDirection);
	bool CanRequestDashLocally() const;
	bool GetPendingDashDirection(FVector& OutDashDirection) const;

	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	void PerformMeleeHit();

	UFUNCTION(BlueprintPure, Category="LS/Combat")
	bool IsAttackInProgress() const;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditDefaultsOnly, Category="LS/Combat")
	TObjectPtr<UAnimMontage> AttackMontage;

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat")
	TSubclassOf<UGameplayAbility> DashAbilityClass;

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat")
	TSubclassOf<UGameplayEffect> BasicAttackDamageEffectClass;

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat", meta=(ClampMin="0.0"))
	float BasicAttackHitDelay = 0.12f;

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat", meta=(ClampMin="0.0"))
	float BasicAttackRecoveryTime = 0.45f;

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat", meta=(ClampMin="0.0"))
	float BasicAttackForwardOffset = 120.0f;

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat", meta=(ClampMin="0.0"))
	float BasicAttackRadius = 100.0f;

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat", meta=(ClampMin="1.0"))
	float DamageEffectLevel = 1.0f;

	FTimerHandle AttackHitTimerHandle;
	FTimerHandle AttackRecoveryTimerHandle;
	FTimerHandle PredictedDashTimerHandle;
	FTimerHandle PredictedDashCooldownTimerHandle;
	bool bAttackHitConsumed = false;
	bool bPredictedDashInProgress = false;
	bool bPredictedDashCooldownActive = false;
	uint16 PredictedDashRootMotionSourceID = 0;
	FVector PendingDashDirection = FVector::ZeroVector;

	ULSAimComponent* ResolveAimComponent() const;
	ULSCharacterCombatComponent* ResolveSharedCombatComponent() const;
	class ALSCharacterBase* ResolveOwnerCharacter() const;
	void FinishAttack();
	void FinishPredictedDash();
	void FinishPredictedDashCooldown();
	void ExecuteMeleeHit(const FVector& AttackDirection);
	bool ApplyDashRootMotion(const FVector& DashDirection, uint16& OutRootMotionSourceID) const;
	float GetDashDuration() const;
	float GetDashCooldown() const;
	bool IsDashCooldownActive() const;
};
