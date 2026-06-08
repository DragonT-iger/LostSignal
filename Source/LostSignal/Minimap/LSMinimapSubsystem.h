#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "LSMinimapSubsystem.generated.h"

class ALSMinimapShapeActor;
class ULSMinimapMarkerComponent;
class ULSMinimapObstacleComponent;

UCLASS()
class LOSTSIGNAL_API ULSMinimapSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void RegisterMarker(ULSMinimapMarkerComponent* Marker);
	void UnregisterMarker(ULSMinimapMarkerComponent* Marker);
	void RegisterShape(ALSMinimapShapeActor* Shape);
	void UnregisterShape(ALSMinimapShapeActor* Shape);
	void RegisterObstacle(ULSMinimapObstacleComponent* Obstacle);
	void UnregisterObstacle(ULSMinimapObstacleComponent* Obstacle);

	void GetRegisteredMarkers(TArray<ULSMinimapMarkerComponent*>& OutMarkers) const;
	void GetRegisteredShapes(TArray<ALSMinimapShapeActor*>& OutShapes) const;
	void GetRegisteredObstacles(TArray<ULSMinimapObstacleComponent*>& OutObstacles) const;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<ULSMinimapMarkerComponent>> Markers;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ALSMinimapShapeActor>> Shapes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULSMinimapObstacleComponent>> Obstacles;
};
