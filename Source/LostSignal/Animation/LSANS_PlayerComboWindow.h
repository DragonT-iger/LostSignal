#pragma once

#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "LSANS_PlayerComboWindow.generated.h"

/**
 * Opens a player basic-attack combo input window.
 * Place this notify state on each combo section where the next attack input should be accepted.
 */
UCLASS()
class LOSTSIGNAL_API ULSANS_PlayerComboWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;
};
