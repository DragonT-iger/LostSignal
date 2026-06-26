#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "LSAN_MonsterActionDash.generated.h"

/**
 * Single-frame dash notify that asks the owning monster combat component to start the active
 * action's data-driven forward dash (Dash_Distance/Duration) on the authored leap frame.
 */
UCLASS()
class LOSTSIGNAL_API ULSAN_MonsterActionDash : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
