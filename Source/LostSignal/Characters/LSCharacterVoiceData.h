#pragma once

#include "Engine/DataAsset.h"
#include "LSCharacterVoiceData.generated.h"

class USoundBase;

/**
 * 캐릭터 음성(보이스) 뱅크. 캐릭터별로 하나씩 두고, 전투 컴포넌트가 피격/사망 시점에 변주를 랜덤 선택해 재생한다.
 * 공격 음성(기합)은 공격 몽타주의 AnimNotify(LSAN_PlaySound)로 처리하므로 여기에 두지 않는다.
 */
UCLASS(BlueprintType)
class LOSTSIGNAL_API ULSCharacterVoiceData : public UDataAsset
{
	GENERATED_BODY()

public:
	// 피격 시 재생할 음성 변주. 재생 시 랜덤 1개 선택(서버에서 골라 복제).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Audio")
	TArray<TObjectPtr<USoundBase>> HitVoices;

	// 사망 시 재생할 음성 변주. 현재 미사용(에셋 추가 시 활성화) — 확장용.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Audio")
	TArray<TObjectPtr<USoundBase>> DeathVoices;

	// 같은 타입 보이스가 이 간격(초) 내에 중복 재생되지 않게 하는 스로틀.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Audio", meta=(ClampMin="0.0"))
	float VoiceMinInterval = 0.25f;
};
