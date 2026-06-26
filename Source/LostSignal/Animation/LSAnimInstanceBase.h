#pragma once

#include "Animation/AnimInstance.h"
#include "LSAnimInstanceBase.generated.h"

class UAbilitySystemComponent;

/**
 * LostSignal AnimBP 공용 부모 클래스.
 * 소유 폰의 ASC 캐싱과 죽음(State_Dead) 판정만 담당한다. 슬롯 블렌딩 같은 특화 로직은 파생 클래스가 추가한다.
 */
UCLASS(Abstract)
class LOSTSIGNAL_API ULSAnimInstanceBase : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY(BlueprintReadOnly, Category="LS/Animation")
	bool bIsDead = false;

protected:
	void CacheAbilitySystemComponent();
	void UpdateDeathState();

	UAbilitySystemComponent* GetCachedAbilitySystemComponent() const { return CachedAbilitySystemComponent.Get(); }

	TWeakObjectPtr<UAbilitySystemComponent> CachedAbilitySystemComponent;
};
