#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "LSAudioSettings.generated.h"

class USoundBase;
class USoundClass;
class USoundMix;

// 사운드 설정(볼륨) 반영에 쓰는 SoundClass/SoundMix 에셋 참조.
// 볼륨 값의 저장·적용 주체는 ULSAudioSettingsSubsystem이다.
UCLASS(config=Game, defaultconfig, meta=(DisplayName="LS Audio Settings"))
class LOSTSIGNAL_API ULSAudioSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	// BGM 버스. BGM 에셋의 Sound Class에 수동 지정한다.
	UPROPERTY(config, EditAnywhere, Category="SoundClasses")
	TSoftObjectPtr<USoundClass> BGMSoundClass;

	// SFX 버스. 프로젝트 설정 > Audio > Default Sound Class로도 지정해
	// Sound Class 미지정 사운드 전부가 자동으로 이 버스를 타게 한다.
	UPROPERTY(config, EditAnywhere, Category="SoundClasses")
	TSoftObjectPtr<USoundClass> SFXSoundClass;

	// 볼륨 오버라이드에 쓰는 SoundMix. Master 볼륨은 C++에서 두 버스에 곱으로 반영한다.
	UPROPERTY(config, EditAnywhere, Category="SoundMix")
	TSoftObjectPtr<USoundMix> VolumeSoundMix;

	// 모든 UMG 버튼의 공통 클릭 사운드. 버튼 스타일에 사운드가 비어 있을 때만 채운다(개별 WBP 지정이 우선).
	UPROPERTY(config, EditAnywhere, Category="ButtonSounds")
	TSoftObjectPtr<USoundBase> ButtonPressedSound;

	// 모든 UMG 버튼의 공통 호버 사운드. 버튼 스타일에 사운드가 비어 있을 때만 채운다(개별 WBP 지정이 우선).
	UPROPERTY(config, EditAnywhere, Category="ButtonSounds")
	TSoftObjectPtr<USoundBase> ButtonHoveredSound;
};
