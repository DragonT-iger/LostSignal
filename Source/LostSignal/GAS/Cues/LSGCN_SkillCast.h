#pragma once

#include "GameplayCueNotify_Static.h"
#include "LSGCN_SkillCast.generated.h"

/**
 * 스킬 시전음 GameplayCue. 스킬 발동 시 서버가 ExecuteGameplayCue로 쏘면 전 클라에서 실행된다.
 * 사운드 에셋을 직접 들고 있지 않고, GameplayCueParameters.SourceObject로 받은 USoundBase를 캐스터 위치에서 재생한다.
 * (스킬별 사운드는 스킬 DataAsset의 CastSound가 단일 출처. 태그는 BP 파생에서 GameplayCue.Skill.Cast로 바인딩.)
 */
UCLASS()
class LOSTSIGNAL_API ULSGCN_SkillCast : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
};
