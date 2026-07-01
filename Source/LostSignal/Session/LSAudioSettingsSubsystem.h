#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LSAudioSettingsSubsystem.generated.h"

UENUM(BlueprintType)
enum class ELSSoundBus : uint8
{
	Master,
	BGM,
	SFX
};

// 사운드 세팅(Master/BGM/SFX) 볼륨 저장소. GameUserSettings.ini에 저장되어 New Game으로도 초기화되지 않는다.
// 실제 오디오 반영(SoundClass/SoundMix)은 관련 에셋이 준비되면 ApplyVolume()에 연결한다.
UCLASS(config=GameUserSettings)
class LOSTSIGNAL_API ULSAudioSettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="LS/Audio")
	float GetVolume(ELSSoundBus Bus) const;

	// 0~1로 클램프해 저장하고 즉시 config에 반영한다.
	UFUNCTION(BlueprintCallable, Category="LS/Audio")
	void SetVolume(ELSSoundBus Bus, float NewVolume);

private:
	UPROPERTY(config)
	float MasterVolume = 1.0f;

	UPROPERTY(config)
	float BGMVolume = 1.0f;

	UPROPERTY(config)
	float SFXVolume = 1.0f;

	// SoundClass/SoundMix 에셋이 아직 없어 실제 오디오 반영은 준비되면 여기서 연결한다.
	void ApplyVolume(ELSSoundBus Bus, float Volume) const;
};
