#pragma once

#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "LSANS_BlockInput.generated.h"

/**
 * 몽타주 구간에 배치해 그 구간 동안만 플레이어 입력을 차단하는 NotifyState.
 * 소유 Actor의 ASC에 LS.State.InputBlocked loose 태그를 NotifyBegin에서 부여, NotifyEnd에서 해제한다.
 * 캐릭터의 전투 입력 핸들러가 이 태그를 확인해 입력을 무시한다(이동·대시·스킬 등).
 * 몽타주에 이 NotifyState가 없으면 스킬 베이스가 몽타주 전체 구간을 기본 차단한다.
 */
UCLASS()
class LOSTSIGNAL_API ULSANS_BlockInput : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
