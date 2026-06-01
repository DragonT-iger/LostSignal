#include "Gameplay/LSNoiseSubsystem.h"

#include "AI/LSMonsterSenseComponent.h"

void ULSNoiseSubsystem::RegisterListener(ULSMonsterSenseComponent* Listener)
{
	if (IsValid(Listener))
	{
		Listeners.Add(Listener);
	}
}

void ULSNoiseSubsystem::UnregisterListener(ULSMonsterSenseComponent* Listener)
{
	if (Listener)
	{
		Listeners.Remove(Listener);
	}
}

void ULSNoiseSubsystem::EmitNoise(const FLSNoiseEvent& NoiseEvent)
{
	if (NoiseEvent.RadiusCm <= 0.0f)
	{
		return;
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
