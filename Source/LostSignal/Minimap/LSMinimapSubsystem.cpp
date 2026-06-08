#include "Minimap/LSMinimapSubsystem.h"

#include "Minimap/LSMinimapMarkerComponent.h"
#include "Minimap/LSMinimapObstacleComponent.h"
#include "Minimap/LSMinimapShapeActor.h"

void ULSMinimapSubsystem::RegisterMarker(ULSMinimapMarkerComponent* Marker)
{
	if (IsValid(Marker))
	{
		Markers.AddUnique(Marker);
	}
}

void ULSMinimapSubsystem::UnregisterMarker(ULSMinimapMarkerComponent* Marker)
{
	Markers.Remove(Marker);
}

void ULSMinimapSubsystem::RegisterShape(ALSMinimapShapeActor* Shape)
{
	if (IsValid(Shape))
	{
		Shapes.AddUnique(Shape);
	}
}

void ULSMinimapSubsystem::UnregisterShape(ALSMinimapShapeActor* Shape)
{
	Shapes.Remove(Shape);
}

void ULSMinimapSubsystem::RegisterObstacle(ULSMinimapObstacleComponent* Obstacle)
{
	if (IsValid(Obstacle))
	{
		Obstacles.AddUnique(Obstacle);
	}
}

void ULSMinimapSubsystem::UnregisterObstacle(ULSMinimapObstacleComponent* Obstacle)
{
	Obstacles.Remove(Obstacle);
}

void ULSMinimapSubsystem::GetRegisteredMarkers(TArray<ULSMinimapMarkerComponent*>& OutMarkers) const
{
	OutMarkers.Reset();
	for (ULSMinimapMarkerComponent* Marker : Markers)
	{
		if (IsValid(Marker))
		{
			OutMarkers.Add(Marker);
		}
	}
}

void ULSMinimapSubsystem::GetRegisteredShapes(TArray<ALSMinimapShapeActor*>& OutShapes) const
{
	OutShapes.Reset();
	for (ALSMinimapShapeActor* Shape : Shapes)
	{
		if (IsValid(Shape))
		{
			OutShapes.Add(Shape);
		}
	}
}

void ULSMinimapSubsystem::GetRegisteredObstacles(TArray<ULSMinimapObstacleComponent*>& OutObstacles) const
{
	OutObstacles.Reset();
	for (ULSMinimapObstacleComponent* Obstacle : Obstacles)
	{
		if (IsValid(Obstacle))
		{
			OutObstacles.Add(Obstacle);
		}
	}
}
