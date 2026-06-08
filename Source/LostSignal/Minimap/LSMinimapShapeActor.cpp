#include "Minimap/LSMinimapShapeActor.h"

#include "Minimap/LSMinimapSubsystem.h"

ALSMinimapShapeActor::ALSMinimapShapeActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ALSMinimapShapeActor::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		if (ULSMinimapSubsystem* MinimapSubsystem = World->GetSubsystem<ULSMinimapSubsystem>())
		{
			MinimapSubsystem->RegisterShape(this);
		}
	}
}

void ALSMinimapShapeActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (ULSMinimapSubsystem* MinimapSubsystem = World->GetSubsystem<ULSMinimapSubsystem>())
		{
			MinimapSubsystem->UnregisterShape(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

FLSMinimapShapeSnapshot ALSMinimapShapeActor::BuildSnapshot() const
{
	FLSMinimapShapeSnapshot Snapshot;
	Snapshot.ShapeType = ShapeType;
	Snapshot.WorldTransform = GetActorTransform();
	Snapshot.Extent = Extent;
	Snapshot.PolylinePoints = PolylinePoints;
	Snapshot.FillColor = FillColor;
	return Snapshot;
}
