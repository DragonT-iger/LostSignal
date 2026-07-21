#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "LSAN_MonsterActionChargeStart.generated.h"

/**
 * 충돌형 돌진 시작 프레임을 MonsterCombatComponent에 전달한다.
 * ChargeStart 섹션에 한 번만 배치하며 ChargeLoop에는 배치하지 않는다.
 */
UCLASS()
class LOSTSIGNAL_API ULSAN_MonsterActionChargeStart : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
