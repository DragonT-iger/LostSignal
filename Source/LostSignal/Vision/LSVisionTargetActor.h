#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LSVisionTargetActor.generated.h"

class UBoxComponent;
class USceneComponent;
class UStaticMeshComponent;
class ULSVisionOccluderComponent;
class ULSVisionSurfaceComponent;

UCLASS()
class LOSTSIGNAL_API ALSVisionTargetActor : public AActor
{
	GENERATED_BODY()

public:
	ALSVisionTargetActor();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LS/Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LS/Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LS/Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> OccluderBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LS/Vision", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULSVisionOccluderComponent> VisionOccluderComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LS/Vision", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULSVisionSurfaceComponent> VisionSurfaceComponent;
};
