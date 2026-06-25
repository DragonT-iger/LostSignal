#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "LSAN_MonsterActionHit.generated.h"

/**
 * Single-frame attack notify that asks the owning monster combat component to apply the active
 * action's data-driven hit on the authored frame.
 */
UCLASS()
class LOSTSIGNAL_API ULSAN_MonsterActionHit : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
