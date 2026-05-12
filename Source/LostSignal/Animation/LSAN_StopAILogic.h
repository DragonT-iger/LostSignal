#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "LSAN_StopAILogic.generated.h"

/**
 * Death montage notify that stops the owning AI brain at an authored frame.
 * Put this near the end of the death montage after the important death pose has started.
 */
UCLASS()
class LOSTSIGNAL_API ULSAN_StopAILogic : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

private:
	UPROPERTY(EditAnywhere, Category="LS/AI")
	FString Reason = TEXT("Death Montage Notify");
};
