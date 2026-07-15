#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "LSAN_Footstep.generated.h"

/**
 * 발 접지 프레임 발소리+이펙트 노티파이. 사운드·VFX는 노티파이가 아니라 캐릭터(ALSCharacterBase::FootstepSound/FootstepVFX)가
 * 소유하므로 같은 애니메이션을 공유해도 캐릭터별 연출이 다르다. 삽입은 tools/insert_footstep_notifies.py가 싱크 마커 위치에 수행.
 */
UCLASS()
class LOSTSIGNAL_API ULSAN_Footstep : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

private:
	// 접지한 발 소켓(본). 사운드 재생 위치이자 향후 VFX·표면 트레이스 확장의 기준점. 없으면 메시 위치에서 재생.
	UPROPERTY(EditAnywhere, Category="LS/Audio")
	FName SocketName = NAME_None;
};
