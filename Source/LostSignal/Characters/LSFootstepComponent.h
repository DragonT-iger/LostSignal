#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "LSFootstepComponent.generated.h"

class USoundBase;

/**
 * 거리 기반 발소리 컴포넌트. 이동 거리 누적이 StrideLength를 넘을 때마다 좌우 발을 번갈아 발소리를 낸다.
 * 애니메이션 마커/커브와 무관해 8방향·걷기·달리기 블렌딩이나 보폭 수 불일치에 영향받지 않는다.
 * (AnimInstance가 아니라 컴포넌트에 두는 이유: movement 도메인이고, AnimInstance는 URO로 업데이트가 throttle될 수 있음.)
 */
UCLASS(ClassGroup=(LS), meta=(BlueprintSpawnableComponent))
class LOSTSIGNAL_API ULSFootstepComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSFootstepComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void PlayFootstep(FName FootBone) const;
	bool IsOwnerDead() const;

	// 발소리 사운드(Sound Cue로 변주). 캐릭터 BP의 컴포넌트 기본값에서 매핑(미할당이면 무음).
	UPROPERTY(EditDefaultsOnly, Category="LS/Audio")
	TObjectPtr<USoundBase> FootstepSound;

	// 한 걸음 사이 이동 거리(cm). 속도가 빠를수록 빨리 차서 발소리도 빨라진다.
	UPROPERTY(EditDefaultsOnly, Category="LS/Audio", meta=(ClampMin="1.0"))
	float StrideLength = 150.0f;

	// 이 속도(cm/s) 미만이면 발소리 안 냄(정지/미세 이동 제외).
	UPROPERTY(EditDefaultsOnly, Category="LS/Audio", meta=(ClampMin="0.0"))
	float MinFootstepSpeed = 10.0f;

	UPROPERTY(EditDefaultsOnly, Category="LS/Audio")
	FName LeftFootBone = "foot_l";

	UPROPERTY(EditDefaultsOnly, Category="LS/Audio")
	FName RightFootBone = "foot_r";

	float FootstepDistanceAccum = 0.0f;
	bool bNextFootLeft = true;
};
