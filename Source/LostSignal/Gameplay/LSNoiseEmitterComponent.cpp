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
		return;
	}

	FLSNoiseEvent NoiseEvent;
	NoiseEvent.NoiseInstigator = OwnerActor;
	NoiseEvent.Location = OwnerActor->GetActorLocation();
	NoiseEvent.RadiusCm = Profile.RadiusMeters * 100.0f;
	NoiseEvent.NoiseTag = Profile.NoiseTag;
	NoiseEvent.bNotifyMonsterSense = bNotifyMonsterSense;

#if !UE_BUILD_SHIPPING
	DrawNoiseDebug(NoiseEvent.Location, NoiseEvent.RadiusCm);
#endif

	if (ULSNoiseSubsystem* NoiseSubsystem = World->GetSubsystem<ULSNoiseSubsystem>())
	{
		NoiseSubsystem->EmitNoise(NoiseEvent);
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
		return nullptr;
	}

	return NoiseProfileTable->FindRow<FLSNoiseProfileRow>(NoiseRowName, TEXT("LSNoiseEmitterComponent"));
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
