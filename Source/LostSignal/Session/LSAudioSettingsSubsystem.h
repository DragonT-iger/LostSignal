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
// 실제 오디오 반영은 ULSAudioSettings의 SoundClass/SoundMix로 ApplyAllVolumes()가 수행한다.
UCLASS(config=GameUserSettings)
class LOSTSIGNAL_API ULSAudioSettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintPure, Category="LS/Audio")
	float GetVolume(ELSSoundBus Bus) const;

	// 0~1로 클램프해 저장하고 즉시 config·오디오에 반영한다.
	UFUNCTION(BlueprintCallable, Category="LS/Audio")
	void SetVolume(ELSSoundBus Bus, float NewVolume);

private:
	UPROPERTY(config)
	float MasterVolume = 1.0f;

	UPROPERTY(config)
	float BGMVolume = 1.0f;

	UPROPERTY(config)
	float SFXVolume = 1.0f;

	// Master는 별도 클래스 오버라이드 없이 BGM/SFX 버스에 곱으로 반영한다(엔진 클래스 계층 중복 적용 회피).
	void ApplyAllVolumes(UWorld* World = nullptr) const;
	void HandlePostLoadMap(UWorld* LoadedWorld);

	FDelegateHandle PostLoadMapHandle;
};
