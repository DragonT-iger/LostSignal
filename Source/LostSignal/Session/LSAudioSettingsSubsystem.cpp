#include "Session/LSAudioSettingsSubsystem.h"

#include "LostSignal.h"

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
	ApplyVolume(Bus, Clamped);
}

void ULSAudioSettingsSubsystem::ApplyVolume(ELSSoundBus Bus, float Volume) const
{
	UE_LOG(LogLS, Verbose, TEXT("[Audio] Bus=%d Volume=%.2f (SoundClass not wired yet)."), static_cast<int32>(Bus), Volume);
}
