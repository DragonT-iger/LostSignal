#pragma once

#include "Combat/LSCombatTypes.h"
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LSPlayerCombatComponent.generated.h"

class UAnimMontage;
class UGameplayAbility;
class UGameplayEffect;
class ULSAimComponent;
class ULSCharacterCombatComponent;
class ULSCombatStateComponent;

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
	bool SubmitDashInput(const FVector& DashDirection, bool& bOutShouldExecuteImmediately);
	bool GetPendingDashDirection(FVector& OutDashDirection) const;

	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	void PerformMeleeHit();

	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	void HandleCombatActionEnd(ELSCombatActionState ExpectedState);

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
	float AttackCancelBlendOutTime = 0.08f;

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat", meta=(ClampMin="0.0"))
	float BasicAttackForwardOffset = 120.0f;

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat", meta=(ClampMin="0.0"))
	float BasicAttackRadius = 100.0f;

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat", meta=(ClampMin="1.0"))
	float DamageEffectLevel = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat", meta=(ClampMin="0.0"))
	float BasicAttackBaseDamage = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat", meta=(ClampMin="0.0"))
	float BasicAttackAttackCoefficient = 0.15f;

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat")
	bool bBasicAttackCanCrit = true;

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat")
	ELSBreakPowerTier BasicAttackBreakPower = ELSBreakPowerTier::NormalAttack;

	FTimerHandle PredictedDashTimerHandle;
	FTimerHandle PredictedDashCooldownTimerHandle;
	bool bAttackHitConsumed = false;
	bool bPredictedDashInProgress = false;
	bool bPredictedDashCooldownActive = false;
	uint16 PredictedDashRootMotionSourceID = 0;
	FVector PendingDashDirection = FVector::ZeroVector;

	ULSAimComponent* ResolveAimComponent() const;
	ULSCharacterCombatComponent* ResolveSharedCombatComponent() const;
	ULSCombatStateComponent* ResolveCombatStateComponent() const;
	class ALSCharacterBase* ResolveOwnerCharacter() const;
	void FinishAttack();
	void CancelAttackForDash();
	void TryExecuteBufferedCommand();
	void FinishPredictedDash();
	void FinishPredictedDashCooldown();
	void ExecuteMeleeHit(const FVector& AttackDirection);
	bool ApplyDashRootMotion(const FVector& DashDirection, uint16& OutRootMotionSourceID) const;
	float GetDashDuration() const;
	float GetDashCooldown() const;
	bool IsDashCooldownActive() const;
};
