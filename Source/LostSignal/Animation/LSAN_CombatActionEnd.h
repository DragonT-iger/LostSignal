#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "Combat/LSCombatTypes.h"
#include "LSAN_CombatActionEnd.generated.h"

UCLASS()
class LOSTSIGNAL_API ULSAN_CombatActionEnd : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

private:
	UPROPERTY(EditAnywhere, Category="LS/Combat")
	ELSCombatActionState ExpectedState = ELSCombatActionState::BasicAttack;
};
