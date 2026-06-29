#pragma once

#include "Animation/LSLocomotionAnimInstance.h"
#include "LSMonsterLocomotionAnimInstance.generated.h"

class UCharacterMovementComponent;

UENUM(BlueprintType)
enum class ELSMonsterLocomotionGait : uint8
{
	Idle,
	Walk,
	Run
};

UCLASS()
class LOSTSIGNAL_API ULSMonsterLocomotionAnimInstance : public ULSLocomotionAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY(BlueprintReadOnly, Category="LS/Animation|Locomotion")
	ELSMonsterLocomotionGait LocomotionGait = ELSMonsterLocomotionGait::Idle;

	UPROPERTY(BlueprintReadOnly, Category="LS/Animation|Locomotion")
	float CurrentMaxWalkSpeed = 0.0f;

	// 속도 멀티플라이어가 적용되지 않은 기준(base) MaxWalkSpeed. ALSEnemyCharacter::GetDefaultMaxWalkSpeed에서 읽음.
	UPROPERTY(BlueprintReadOnly, Category="LS/Animation|Locomotion")
	float BaseMaxWalkSpeed = 0.0f;

	// 이 속력(velocity) 이하이면 Idle.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Animation|Locomotion", meta=(ClampMin="0.0"))
	float MovingSpeedThreshold = 3.0f;

	// 아키타입 속도 멀티플라이어가 없을 때 walk/run을 가르는 기준 비율(base 대비).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Animation|Locomotion", meta=(ClampMin="0.0", ClampMax="1.0"))
	float WalkRunThresholdAlpha = 0.85f;

private:
	void UpdateMovementSpeedCache();
	void UpdateLocomotionGait();
	float ResolveWalkRunMaxSpeedThreshold() const;
	const UCharacterMovementComponent* GetOwnerMovementComponent() const;
};
