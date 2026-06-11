#include "Gameplay/LSNoiseSubsystem.h"

#include "AI/LSMonsterSenseComponent.h"
#include "Core/LSPlayerControllerBase.h"
#include "Engine/World.h"
#include "LostSignal.h"

namespace
{
bool ShouldNotifyPlayerController(const ALSPlayerControllerBase* PlayerController, const FLSNoiseEvent& NoiseEvent)
{
	if (!PlayerController)
	{
		return false;
	}

	const APawn* Pawn = PlayerController->GetPawn();
	if (!Pawn || Pawn == NoiseEvent.NoiseInstigator)
	{
		return false;
	}

	const float HUDRadiusCm = PlayerController->GetSoundIndicatorDetectionRadiusCm();
	return HUDRadiusCm > 0.0f && FVector::DistSquared(Pawn->GetActorLocation(), NoiseEvent.Location) <= FMath::Square(HUDRadiusCm);
}
}

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

	if (NoiseEvent.bNotifyMonsterSense)
	{
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

	UWorld* World = GetWorld();
	UE_LOG(LogLS, Warning, TEXT("[SoundIndicator] NoiseSubsystem received. Instigator=%s Location=%s RadiusCm=%.2f NotifyMonsterSense=%d PlayerControllers=%d"),
		*GetNameSafe(NoiseEvent.NoiseInstigator),
		*NoiseEvent.Location.ToCompactString(),
		NoiseEvent.RadiusCm,
		NoiseEvent.bNotifyMonsterSense,
		World ? World->GetNumPlayerControllers() : 0);

	if (!World)
	{
		return;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(It->Get());
		if (ShouldNotifyPlayerController(PlayerController, NoiseEvent))
		{
			UE_LOG(LogLS, Warning, TEXT("[SoundIndicator] NoiseSubsystem notify player. PC=%s Pawn=%s"),
				*GetNameSafe(PlayerController),
				*GetNameSafe(PlayerController ? PlayerController->GetPawn() : nullptr));
			PlayerController->NotifyNoiseForHUD(NoiseEvent);
		}
		else
		{
			const APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
			const float Dist = Pawn ? FVector::Dist(Pawn->GetActorLocation(), NoiseEvent.Location) : -1.0f;
			UE_LOG(LogLS, Warning, TEXT("[SoundIndicator] NoiseSubsystem skip player. PC=%s Pawn=%s Distance=%.2f RadiusCm=%.2f HUDDetectionRadiusCm=%.2f Instigator=%s"),
				*GetNameSafe(PlayerController),
				*GetNameSafe(Pawn),
				Dist,
				NoiseEvent.RadiusCm,
				PlayerController ? PlayerController->GetSoundIndicatorDetectionRadiusCm() : 0.0f,
				*GetNameSafe(NoiseEvent.NoiseInstigator));
		}
	}
}
