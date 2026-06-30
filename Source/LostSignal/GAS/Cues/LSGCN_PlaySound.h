#pragma once

#include "GameplayCueNotify_Static.h"
#include "LSGCN_PlaySound.generated.h"

/**
 * 범용 사운드 재생 GameplayCue. GameplayCueParameters.SourceObject로 받은 USoundBase를 MyTarget(큐 대상) 위치에서 재생한다.
 * 사운드를 직접 들지 않고 호출부가 파라미터로 넘기므로, 하나의 클래스로 여러 태그를 처리한다:
 *   - GameplayCue.Combat.Hit (피격 임팩트음, 재질별 — 피격자 데이터)
 *   - GameplayCue.Voice      (피격 보이스 — 피격자 데이터)
 *   - GameplayCue.Skill.Cast (스킬 시전음 — 스킬 DataAsset)
 * 태그는 각 BP 파생에서 GameplayCueTag로 바인딩한다.
 */
UCLASS()
class LOSTSIGNAL_API ULSGCN_PlaySound : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
};
