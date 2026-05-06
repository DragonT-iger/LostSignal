#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "LSAN_PlayerMeleeHit.generated.h"

UCLASS()
class LOSTSIGNAL_API ULSAN_PlayerMeleeHit : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
