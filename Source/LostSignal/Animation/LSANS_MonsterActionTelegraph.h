#pragma once

#include "AI/LSMonsterCombatComponent.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "LSANS_MonsterActionTelegraph.generated.h"

/**
 * Windup notify-state that shows the active action's attack range as a telegraph for its duration.
 * Begin → BeginActionTelegraph, End → EndActionTelegraph on the owning monster combat component.
 * Lets designers control telegraph timing/length directly on the montage.
 */
UCLASS()
class LOSTSIGNAL_API ULSANS_MonsterActionTelegraph : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

private:
	/** 텔레그래프를 배치할 월드 위치 기준. Notify Begin 시점 위치를 사용하며 기본은 시전자다. */
	UPROPERTY(EditAnywhere, Category="LS/Combat|Telegraph")
	ELSMonsterTelegraphOrigin OriginMode = ELSMonsterTelegraphOrigin::Caster;
};
