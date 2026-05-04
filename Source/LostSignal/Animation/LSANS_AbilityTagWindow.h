#pragma once

#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "LSANS_AbilityTagWindow.generated.h"

/**
 * Notify state that grants a loose gameplay tag for a montage-authored window.
 * Use this for phases like AttackActive that other systems need to query.
 */
UCLASS()
class LOSTSIGNAL_API ULSANS_AbilityTagWindow : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	// NotifyBegin/NotifyEnd mirror the montage-authored window onto ASC loose tags for simple state queries.
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

protected:
	UPROPERTY(EditAnywhere, Category="LS/Combat")
	FGameplayTag GrantedTag;
};
