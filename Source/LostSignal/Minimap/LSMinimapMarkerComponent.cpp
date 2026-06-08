#include "Minimap/LSMinimapMarkerComponent.h"

#include "Minimap/LSMinimapSubsystem.h"

ULSMinimapMarkerComponent::ULSMinimapMarkerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULSMinimapMarkerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		if (ULSMinimapSubsystem* MinimapSubsystem = World->GetSubsystem<ULSMinimapSubsystem>())
		{
			MinimapSubsystem->RegisterMarker(this);
		}
	}
}

void ULSMinimapMarkerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (ULSMinimapSubsystem* MinimapSubsystem = World->GetSubsystem<ULSMinimapSubsystem>())
		{
			MinimapSubsystem->UnregisterMarker(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

FLSMinimapMarkerSnapshot ULSMinimapMarkerComponent::BuildSnapshot() const
{
	FLSMinimapMarkerSnapshot Snapshot;
	AActor* Owner = GetOwner();
	Snapshot.OwnerActor = Owner;
	Snapshot.MarkerType = MarkerType;
	Snapshot.WorldLocation = Owner ? Owner->GetActorLocation() : FVector::ZeroVector;
	Snapshot.Color = MarkerColor;
	Snapshot.DrawRadius = DrawRadius;
	Snapshot.Priority = Priority;
	Snapshot.bAlwaysVisible = bAlwaysVisible;
	Snapshot.bVisible = bVisibleOnMinimap;
	return Snapshot;
}
