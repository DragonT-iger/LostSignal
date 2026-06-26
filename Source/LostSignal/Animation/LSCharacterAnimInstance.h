#pragma once

#include "Animation/LSAnimInstanceBase.h"
#include "LSCharacterAnimInstance.generated.h"

class UAnimMontage;

/**
 * 상하체 슬롯 블렌딩을 사용하는 캐릭터용 AnimBP 부모 클래스.
 * 공용 ASC 캐싱/죽음 판정은 ULSAnimInstanceBase가 담당하고, 여기서는 슬롯 블렌딩 플래그만 갱신한다.
 */
UCLASS()
class LOSTSIGNAL_API ULSCharacterAnimInstance : public ULSAnimInstanceBase
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
	bool bIsFullBodyMontagePlaying = false;

	UPROPERTY(BlueprintReadOnly, Category="LS/Animation")
	bool bShouldBlockUpperBodySlot = false;

	UPROPERTY(BlueprintReadOnly, Category="LS/Animation")
	TObjectPtr<UAnimMontage> CurrentActiveMontage = nullptr;

protected:
	UFUNCTION(BlueprintPure, Category="LS/Animation")
	bool IsMontageUsingSlot(const UAnimMontage* Montage, FName SlotName) const;
};
