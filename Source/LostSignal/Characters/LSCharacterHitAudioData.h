#pragma once

#include "Engine/DataAsset.h"
#include "LSCharacterHitAudioData.generated.h"

class USoundBase;

/**
 * 피격 시 재생할 캐릭터/몬스터별 오디오 묶음. 피격자가 자기 데이터를 가지므로 종류·재질별 분기가 데이터 교체만으로 된다.
 * 같은 재질(예: 살)은 여러 적이 같은 Sound Cue를 참조해 공유한다. 공격 음성(기합)은 몽타주 AnimNotify가 담당하므로 여기 없음.
 */
UCLASS(BlueprintType)
class LOSTSIGNAL_API ULSCharacterHitAudioData : public UDataAsset
{
	GENERATED_BODY()

public:
	// 재질 임팩트음(퍽/챙/물컹). 피격 시 1회 재생.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Audio")
	TObjectPtr<USoundBase> HitImpactSound;

	// 피격 보이스 변주. 재생 시 랜덤 1개(서버 선택 후 복제).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Audio")
	TArray<TObjectPtr<USoundBase>> HitVoices;

	// 사망 보이스 변주. 현재 미사용(에셋 추가 시 활성화) — 확장용.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Audio")
	TArray<TObjectPtr<USoundBase>> DeathVoices;

	// 임팩트음 최소 재생 간격(초). 0이면 매 피격마다 재생.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Audio", meta=(ClampMin="0.0"))
	float ImpactMinInterval = 0.0f;

	// 보이스 최소 재생 간격(초). 연타 피격 시 도배 방지.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Audio", meta=(ClampMin="0.0"))
	float VoiceMinInterval = 0.25f;
};
