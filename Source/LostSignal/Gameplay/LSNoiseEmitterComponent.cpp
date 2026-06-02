#include "Gameplay/LSNoiseEmitterComponent.h"

#include "Characters/LSPlayerCharacter.h"
#include "Data/LSNoiseProfileRow.h"
#include "DrawDebugHelpers.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Gameplay/LSNoiseSubsystem.h"
#include "Gameplay/LSNoiseTypes.h"
#include "LostSignal.h"

namespace
{
	FString GetNoiseTagDebugString(const FLSNoiseProfileRow& Profile)
	{
		return Profile.NoiseTag.IsValid() ? Profile.NoiseTag.ToString() : TEXT("None");
	}
}

ULSNoiseEmitterComponent::ULSNoiseEmitterComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;
}

void ULSNoiseEmitterComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	UpdateMovementNoise(DeltaTime);
}

void ULSNoiseEmitterComponent::EmitNoiseByRow(FName NoiseRowName)
{
	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		if (bLogNoiseDebug)
		{
			UE_LOG(LogLS, Warning, TEXT("[NoiseEmitter] Emit skipped. Owner=%s HasAuthority=%d Row=%s"),
				*GetNameSafe(OwnerActor),
				OwnerActor ? OwnerActor->HasAuthority() : false,
				*NoiseRowName.ToString());
		}
		return;
	}

	if (const FLSNoiseProfileRow* Profile = FindNoiseProfile(NoiseRowName))
	{
		EmitNoiseFromProfile(*Profile);
	}
}

void ULSNoiseEmitterComponent::EmitInteractNoise()
{
	EmitNoiseByRow(InteractNoiseRowName);
}

void ULSNoiseEmitterComponent::UpdateMovementNoise(float DeltaTime)
{
	if (!IsOwnerMoving())
	{
		MovementNoiseElapsedSeconds = 0.0f;
		return;
	}

	const FName NoiseRowName = IsOwnerRunning() ? RunNoiseRowName : WalkNoiseRowName;
	const FLSNoiseProfileRow* Profile = FindNoiseProfile(NoiseRowName);
	if (!Profile || Profile->EmitIntervalSeconds <= 0.0f)
	{
		if (bLogNoiseDebug && !Profile)
		{
			UE_LOG(LogLS, Warning, TEXT("[NoiseEmitter] Movement noise profile missing. Owner=%s Row=%s"),
				*GetNameSafe(GetOwner()),
				*NoiseRowName.ToString());
		}
		return;
	}

	MovementNoiseElapsedSeconds += DeltaTime;
	if (MovementNoiseElapsedSeconds < Profile->EmitIntervalSeconds)
	{
		return;
	}

	MovementNoiseElapsedSeconds = 0.0f;
	EmitNoiseFromProfile(*Profile);
}

void ULSNoiseEmitterComponent::EmitNoiseFromProfile(const FLSNoiseProfileRow& Profile)
{
	AActor* OwnerActor = GetOwner();
	UWorld* World = GetWorld();
	if (!OwnerActor || !World || Profile.RadiusMeters <= 0.0f)
	{
		if (bLogNoiseDebug)
		{
			UE_LOG(LogLS, Warning, TEXT("[NoiseEmitter] Noise skipped. Owner=%s World=%d RadiusMeters=%.2f Tag=%s"),
				*GetNameSafe(OwnerActor),
				World != nullptr,
				Profile.RadiusMeters,
				*GetNoiseTagDebugString(Profile));
		}
		return;
	}

	FLSNoiseEvent NoiseEvent;
	NoiseEvent.NoiseInstigator = OwnerActor;
	NoiseEvent.Location = OwnerActor->GetActorLocation();
	NoiseEvent.RadiusCm = Profile.RadiusMeters * 100.0f;
	NoiseEvent.NoiseTag = Profile.NoiseTag;

#if !UE_BUILD_SHIPPING
	DrawNoiseDebug(NoiseEvent.Location, NoiseEvent.RadiusCm);
#endif

	if (bLogNoiseDebug)
	{
		UE_LOG(LogLS, Warning, TEXT("[NoiseEmitter] Noise emitted. Owner=%s Tag=%s Location=%s RadiusCm=%.2f"),
			*GetNameSafe(OwnerActor),
			*GetNoiseTagDebugString(Profile),
			*NoiseEvent.Location.ToCompactString(),
			NoiseEvent.RadiusCm);
	}

	if (ULSNoiseSubsystem* NoiseSubsystem = World->GetSubsystem<ULSNoiseSubsystem>())
	{
		NoiseSubsystem->EmitNoise(NoiseEvent);
	}
	else if (bLogNoiseDebug)
	{
		UE_LOG(LogLS, Warning, TEXT("[NoiseEmitter] NoiseSubsystem missing. Owner=%s"), *GetNameSafe(OwnerActor));
	}
}

const FLSNoiseProfileRow* ULSNoiseEmitterComponent::FindNoiseProfile(FName NoiseRowName) const
{
	if (!NoiseProfileTable)
	{
		const_cast<ULSNoiseEmitterComponent*>(this)->LogMissingNoiseProfileTableOnce();
		return nullptr;
	}

	if (NoiseRowName.IsNone())
	{
		if (bLogNoiseDebug)
		{
			UE_LOG(LogLS, Warning, TEXT("[NoiseEmitter] Noise row name is None. Owner=%s"), *GetNameSafe(GetOwner()));
		}
		return nullptr;
	}

	const FLSNoiseProfileRow* Row = NoiseProfileTable->FindRow<FLSNoiseProfileRow>(NoiseRowName, TEXT("LSNoiseEmitterComponent"));
	if (!Row && bLogNoiseDebug)
	{
		UE_LOG(LogLS, Warning, TEXT("[NoiseEmitter] Noise row not found. Owner=%s Table=%s Row=%s"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(NoiseProfileTable),
			*NoiseRowName.ToString());
	}

	return Row;
}

bool ULSNoiseEmitterComponent::IsOwnerMoving() const
{
	const AActor* OwnerActor = GetOwner();
	const FVector Velocity = OwnerActor ? OwnerActor->GetVelocity() : FVector::ZeroVector;
	return Velocity.SizeSquared2D() > FMath::Square(MinimumMovementSpeed);
}

bool ULSNoiseEmitterComponent::IsOwnerRunning() const
{
	const ALSPlayerCharacter* PlayerCharacter = Cast<ALSPlayerCharacter>(GetOwner());
	return PlayerCharacter && PlayerCharacter->IsRunning();
}

void ULSNoiseEmitterComponent::DrawNoiseDebug(const FVector& Location, float RadiusCm) const
{
	if (!bDrawNoiseDebug || RadiusCm <= 0.0f)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector DebugLocation = Location + FVector(0.0f, 0.0f, NoiseDebugDrawHeight);
	DrawDebugCircle(World, DebugLocation, RadiusCm, 48, FColor::Cyan, false, NoiseDebugDuration, 0, 2.0f, FVector::ForwardVector, FVector::RightVector, false);
	DrawDebugSphere(World, DebugLocation, 16.0f, 8, FColor::Cyan, false, NoiseDebugDuration);
}

void ULSNoiseEmitterComponent::LogMissingNoiseProfileTableOnce()
{
	if (bLoggedMissingNoiseProfileTable)
	{
		return;
	}

	bLoggedMissingNoiseProfileTable = true;
	UE_LOG(LogLS, Warning, TEXT("NoiseProfileTable is not set on %s."), *GetNameSafe(this));
}
