#pragma once

#include "Animation/LSAnimInstanceBase.h"
#include "LSLocomotionAnimInstance.generated.h"

/**
 * 상하체 블렌딩이 필요 없는 캐릭터용 로코모션 AnimBP 부모 클래스.
 * Event Blueprint Update Animation 그래프(속도/속력/방향/Yaw 산출)를 C++로 대체한다.
 */
UCLASS()
class LOSTSIGNAL_API ULSLocomotionAnimInstance : public ULSAnimInstanceBase
{
	GENERATED_BODY()

public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	// 소유 폰의 현재 속도 벡터
	UPROPERTY(BlueprintReadOnly, Category="LS/Animation|Locomotion")
	FVector CurrentVelocity = FVector::ZeroVector;

	// 속도 크기(BlendSpace 이동 속도용)
	UPROPERTY(BlueprintReadOnly, Category="LS/Animation|Locomotion")
	float Speed = 0.0f;

	// 폰 정면 기준 이동 방향 각도(-180~180, CalculateDirection 결과)
	UPROPERTY(BlueprintReadOnly, Category="LS/Animation|Locomotion")
	float Direction = 0.0f;

private:
	void UpdateLocomotionData();
};
