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

	// 이동속도 어트리뷰트 배수(기본 1.0). 로코모션 BlendSpace 재생속도(Play Rate)에 연결해
	// MaxWalkSpeed 배수와 애니메이션 재생속도를 동기화(발 미끄러짐 방지)한다. 어트리뷰트가 없으면 1.0.
	UPROPERTY(BlueprintReadOnly, Category="LS/Animation")
	float MoveSpeedMultiplier = 1.0f;

protected:
	void CacheAbilitySystemComponent();
	void UpdateDeathState();
	void UpdateMoveSpeedMultiplier();

	UAbilitySystemComponent* GetCachedAbilitySystemComponent() const { return CachedAbilitySystemComponent.Get(); }

	TWeakObjectPtr<UAbilitySystemComponent> CachedAbilitySystemComponent;
};
