#include "Gameplay/LSNoiseSubsystem.h"

#include "AI/LSMonsterSenseComponent.h"
#include "LostSignal.h"

void ULSNoiseSubsystem::RegisterListener(ULSMonsterSenseComponent* Listener)
{
	if (IsValid(Listener))
	{
		Listeners.Add(Listener);
		if (bLogNoiseDebug)
		{
			UE_LOG(LogLS, Warning, TEXT("[NoiseSubsystem] Listener registered. Listener=%s Count=%d"),
				*GetNameSafe(Listener),
				Listeners.Num());
		}
	}
}

void ULSNoiseSubsystem::UnregisterListener(ULSMonsterSenseComponent* Listener)
{
	if (Listener)
	{
		Listeners.Remove(Listener);
		if (bLogNoiseDebug)
		{
			UE_LOG(LogLS, Warning, TEXT("[NoiseSubsystem] Listener unregistered. Listener=%s Count=%d"),
				*GetNameSafe(Listener),
				Listeners.Num());
		}
	}
}

void ULSNoiseSubsystem::EmitNoise(const FLSNoiseEvent& NoiseEvent)
{
	if (NoiseEvent.RadiusCm <= 0.0f)
	{
		if (bLogNoiseDebug)
		{
			UE_LOG(LogLS, Warning, TEXT("[NoiseSubsystem] Noise ignored because radius is invalid. Instigator=%s RadiusCm=%.2f"),
				*GetNameSafe(NoiseEvent.NoiseInstigator),
				NoiseEvent.RadiusCm);
		}
		return;
	}

	if (bLogNoiseDebug)
	{
		UE_LOG(LogLS, Warning, TEXT("[NoiseSubsystem] Noise dispatch. Instigator=%s Tag=%s Location=%s RadiusCm=%.2f ListenerCount=%d"),
			*GetNameSafe(NoiseEvent.NoiseInstigator),
			NoiseEvent.NoiseTag.IsValid() ? *NoiseEvent.NoiseTag.ToString() : TEXT("None"),
			*NoiseEvent.Location.ToCompactString(),
			NoiseEvent.RadiusCm,
			Listeners.Num());
	}

	for (auto It = Listeners.CreateIterator(); It; ++It)
	{
		ULSMonsterSenseComponent* Listener = It->Get();
		if (!IsValid(Listener))
		{
			It.RemoveCurrent();
			continue;
		}

		Listener->RegisterNoiseEvent(NoiseEvent);
	}
}
