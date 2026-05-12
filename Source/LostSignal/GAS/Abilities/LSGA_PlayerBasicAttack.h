#pragma once

#include "Abilities/GameplayAbility.h"
#include "LSGA_PlayerBasicAttack.generated.h"

class UAbilitySystemComponent;
class UAnimMontage;

/**
 * Player basic attack combo ability.
 * Input starts the ability once, combo-window notifies decide whether to jump to the next montage section.
 */
UCLASS()
class LOSTSIGNAL_API ULSGA_PlayerBasicAttack : public UGameplayAbility
{
	GENERATED_BODY()

public:
	ULSGA_PlayerBasicAttack();

	static ULSGA_PlayerBasicAttack* FindActiveBasicAttackAbility(UAbilitySystemComponent* ASC);

	void QueueComboInput();
	void OpenComboWindow();
	void CloseComboWindow();

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
	void PlayComboSection(int32 SectionIndex);
	void OpenPostComboInputWindow();
	void ClosePostComboInputWindow();
	void ConsumePostComboInput();
	void HandleAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	void SetComboWindowTagActive(bool bActive);

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat")
	TArray<FName> ComboSections;

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat", meta=(ClampMin="0.0"))
	float CancelBlendOutTime = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat", meta=(ClampMin="0.0"))
	float PostComboInputWindowSeconds = 0.35f;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveAttackMontage = nullptr;

	FTimerHandle PostComboInputWindowTimerHandle;
	int32 CurrentSectionIndex = INDEX_NONE;
	bool bComboInputBuffered = false;
	bool bComboWindowOpen = false;
	bool bWaitingForPostComboInput = false;
	bool bComboWindowTagActive = false;
	bool bEndingAbility = false;
};
