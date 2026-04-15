#include "Vision/LSVisionTargetActor.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Vision/LSVisionOccluderComponent.h"
#include "Vision/LSVisionSurfaceComponent.h"

// Convenience actor that ships with the minimum components needed to behave as an occluding vision surface.
ALSVisionTargetActor::ALSVisionTargetActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(SceneRoot);
	VisualMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);

	OccluderBox = CreateDefaultSubobject<UBoxComponent>(TEXT("OccluderBox"));
	OccluderBox->SetupAttachment(SceneRoot);
	OccluderBox->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	OccluderBox->SetBoxExtent(FVector(50.0f, 50.0f, 100.0f));

	VisionOccluderComponent = CreateDefaultSubobject<ULSVisionOccluderComponent>(TEXT("VisionOccluderComponent"));
	VisionOccluderComponent->SourceMode = ELSVisionOccluderSourceMode::BoxComponent;
	VisionOccluderComponent->SourceBoxComponent = OccluderBox;

	VisionSurfaceComponent = CreateDefaultSubobject<ULSVisionSurfaceComponent>(TEXT("VisionSurfaceComponent"));
	VisionSurfaceComponent->TargetPrimitives.Add(VisualMesh);
}
