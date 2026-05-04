#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "LSAN_MonsterMeleeHit.generated.h"

/**
 * Single-frame attack notify that asks the owning monster combat component to apply melee damage.
 */
UCLASS()
class LOSTSIGNAL_API ULSAN_MonsterMeleeHit : public UAnimNotify
{
	GENERATED_BODY()

public:
	// Montage notify entry point used to trigger the monster's melee hit check on the authored frame.
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
