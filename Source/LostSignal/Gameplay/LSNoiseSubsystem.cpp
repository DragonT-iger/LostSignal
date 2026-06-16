#include "Gameplay/LSNoiseSubsystem.h"

#include "AI/LSMonsterSenseComponent.h"
#include "Characters/LSPlayerCharacter.h"
#include "Core/LSPlayerControllerBase.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

namespace
{
bool IsNoiseInstigatorPlayerForHUDNotification(const AActor* NoiseInstigator)
{
	if (!NoiseInstigator)
	{
		return false;
	}

	if (NoiseInstigator->IsA<ALSPlayerCharacter>())
	{
		return true;
	}

	const APawn* InstigatorPawn = Cast<APawn>(NoiseInstigator);
	return InstigatorPawn && InstigatorPawn->IsPlayerControlled();
}

bool ShouldNotifyPlayerController(const ALSPlayerControllerBase* PlayerController, const FLSNoiseEvent& NoiseEvent)
{
	if (!PlayerController || IsNoiseInstigatorPlayerForHUDNotification(NoiseEvent.NoiseInstigator))
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
	if (!World)
	{
		return;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(It->Get());
		if (ShouldNotifyPlayerController(PlayerController, NoiseEvent))
		{
			PlayerController->NotifyNoiseForHUD(NoiseEvent);
		}
	}
}
