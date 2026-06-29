#pragma once

#include "GameplayCueNotify_Static.h"
#include "LSGCN_Hit.generated.h"

class USoundBase;
class USoundAttenuation;
class USoundConcurrency;

/**
 * 피격 일회성 GameplayCue. 데미지 GE(Instant)가 대상에 적용될 때 OnExecute가 불려 피격자 위치에서 피격음을 재생한다.
 * 사운드 에셋과 GameplayCueTag(=GameplayCue.Combat.Hit)는 BP 파생에서 매핑한다(경로 하드코딩 금지).
 */
UCLASS()
class LOSTSIGNAL_API ULSGCN_Hit : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;

private:
	UPROPERTY(EditDefaultsOnly, Category="LS/Audio")
	TObjectPtr<USoundBase> HitSound;

	UPROPERTY(EditDefaultsOnly, Category="LS/Audio")
	float VolumeMultiplier = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category="LS/Audio")
	float PitchMultiplier = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category="LS/Audio")
	TObjectPtr<USoundAttenuation> AttenuationSettings;

	UPROPERTY(EditDefaultsOnly, Category="LS/Audio")
	TObjectPtr<USoundConcurrency> ConcurrencySettings;
};
