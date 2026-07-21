#pragma once

#include "Abilities/GameplayAbility.h"
#include "LSGA_MonsterAction.generated.h"

class UAnimMontage;
class ULSMonsterCombatComponent;

/**
 * Data-driven monster attack ability.
 * StateTree picks an action via ULSMonsterCombatComponent::RequestAction; this ability reads the
 * active FLSMonsterActionRow, plays the row's authored montage (Action_Ani), and authored frame
 * notifies drive the telegraph and the hit. Replaces the fixed monster melee ability.
 */
UCLASS()
class LOSTSIGNAL_API ULSGA_MonsterAction : public UGameplayAbility
{
	GENERATED_BODY()

public:
	ULSGA_MonsterAction();

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

private:
	void HandleActionMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void HandleActionChargeStarted();
	void HandleActionChargeFinished(bool bHit);
	void BindActionChargeDelegates(ULSMonsterCombatComponent* CombatComponent);
	void UnbindActionChargeDelegates();
	bool HasValidActionChargeSections() const;

	/** Montage resolved from the active action row (Action_Ani) when the ability activates. */
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveActionMontage;

	TWeakObjectPtr<ULSMonsterCombatComponent> ActiveCombatComponent;
	FDelegateHandle ActionChargeStartedHandle;
	FDelegateHandle ActionChargeFinishedHandle;

	bool bEndingAbility = false;

	// 공격 중에는 플레이어를 향한 body 회전을 멈춰 공격 방향을 고정한다(종료 시 복원).
	bool bSavedUseControllerDesiredRotation = true;
};
