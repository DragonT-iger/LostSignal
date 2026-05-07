#pragma once

#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Combat/LSCombatTypes.h"
#include "LSANS_CombatActionPhase.generated.h"

UCLASS()
class LOSTSIGNAL_API ULSANS_CombatActionPhase : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

private:
	UPROPERTY(EditAnywhere, Category="LS/Combat")
	ELSCombatActionPhase Phase = ELSCombatActionPhase::Startup;
};
