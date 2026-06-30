#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "LSAN_SkillEffect.generated.h"

/**
 * 스킬 몽타주의 효과 발동 프레임에 배치하는 노티파이.
 * 캐스터에게 GameplayEvent(기본 LS.Event.Skill.Hit)를 보내, 대기 중인 스킬 Ability가 효과를 실행하게 한다.
 * 노티파이는 특정 Ability 클래스를 알지 못하고 태그만 보낸다(결합도 0, 다단 히트 확장 가능).
 */
UCLASS()
class LOSTSIGNAL_API ULSAN_SkillEffect : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

protected:
	// 발동할 스킬 효과 이벤트 태그. 비워두면 LS.Event.Skill.Hit를 사용한다.
	UPROPERTY(EditAnywhere, Category="LS/Skill")
	FGameplayTag SkillEffectEventTag;
};
