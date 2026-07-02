#include "Session/LSAudioSettingsSubsystem.h"

#include "Data/LSAudioSettings.h"
#include "Kismet/GameplayStatics.h"
#include "LostSignal.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "UObject/UObjectGlobals.h"

void ULSAudioSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 믹스 오버라이드는 월드(오디오 디바이스) 단위라 첫 맵 로드를 포함해 맵이 바뀔 때마다 다시 밀어넣는다.
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ULSAudioSettingsSubsystem::HandlePostLoadMap);
}

void ULSAudioSettingsSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
	Super::Deinitialize();
}

float ULSAudioSettingsSubsystem::GetVolume(ELSSoundBus Bus) const
{
	switch (Bus)
	{
	case ELSSoundBus::Master: return MasterVolume;
	case ELSSoundBus::BGM: return BGMVolume;
	case ELSSoundBus::SFX: return SFXVolume;
	}
	return 1.0f;
}

void ULSAudioSettingsSubsystem::SetVolume(ELSSoundBus Bus, float NewVolume)
{
	const float Clamped = FMath::Clamp(NewVolume, 0.0f, 1.0f);
	switch (Bus)
	{
	case ELSSoundBus::Master: MasterVolume = Clamped; break;
	case ELSSoundBus::BGM: BGMVolume = Clamped; break;
	case ELSSoundBus::SFX: SFXVolume = Clamped; break;
	}

	SaveConfig();
	// Master 변경이 BGM/SFX 실효값에 함께 반영되도록 항상 전체를 다시 적용한다.
	ApplyAllVolumes();
}

void ULSAudioSettingsSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	// PIE 다중 인스턴스 등 다른 GameInstance의 맵 로드에는 반응하지 않는다.
	if (LoadedWorld && LoadedWorld->GetGameInstance() == GetGameInstance())
	{
		ApplyAllVolumes(LoadedWorld);
	}
}

void ULSAudioSettingsSubsystem::ApplyAllVolumes(UWorld* World) const
{
	if (!World)
	{
		World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	}
	if (!World)
	{
		return;
	}

	const ULSAudioSettings* AudioSettings = GetDefault<ULSAudioSettings>();
	USoundMix* VolumeMix = AudioSettings ? AudioSettings->VolumeSoundMix.LoadSynchronous() : nullptr;
	USoundClass* BGMClass = AudioSettings ? AudioSettings->BGMSoundClass.LoadSynchronous() : nullptr;
	USoundClass* SFXClass = AudioSettings ? AudioSettings->SFXSoundClass.LoadSynchronous() : nullptr;
	if (!VolumeMix || !BGMClass || !SFXClass)
	{
		UE_LOG(LogLS, Warning, TEXT("LS Audio Settings SoundClass/SoundMix is not assigned; cannot apply volume settings."));
		return;
	}

	UGameplayStatics::SetSoundMixClassOverride(World, VolumeMix, BGMClass, MasterVolume * BGMVolume, 1.0f, 0.0f, true);
	UGameplayStatics::SetSoundMixClassOverride(World, VolumeMix, SFXClass, MasterVolume * SFXVolume, 1.0f, 0.0f, true);
	UGameplayStatics::PushSoundMixModifier(World, VolumeMix);
}
