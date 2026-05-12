#pragma once

#include "Animation/AnimInstance.h"
#include "LSCharacterAnimInstance.generated.h"

class UAbilitySystemComponent;
class UAnimMontage;

/**
 * AnimBP parent class for LostSignal characters.
 * Exposes GAS/state-driven animation flags so AnimGraph can decide slot blending without BP event-graph logic.
 */
UCLASS()
class LOSTSIGNAL_API ULSCharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category="LS/Animation")
	void RefreshAnimationState();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Animation")
	FName FullBodySlotName = TEXT("FullBody");

	UPROPERTY(BlueprintReadOnly, Category="LS/Animation")
	bool bIsDead = false;

	UPROPERTY(BlueprintReadOnly, Category="LS/Animation")
	bool bIsFullBodyMontagePlaying = false;

	UPROPERTY(BlueprintReadOnly, Category="LS/Animation")
	bool bShouldBlockUpperBodySlot = false;

	UPROPERTY(BlueprintReadOnly, Category="LS/Animation")
	TObjectPtr<UAnimMontage> CurrentActiveMontage = nullptr;

protected:
	UFUNCTION(BlueprintPure, Category="LS/Animation")
	bool IsMontageUsingSlot(const UAnimMontage* Montage, FName SlotName) const;

private:
	void CacheAbilitySystemComponent();

	TWeakObjectPtr<UAbilitySystemComponent> CachedAbilitySystemComponent;
};
