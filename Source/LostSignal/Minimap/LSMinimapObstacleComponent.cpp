#include "Minimap/LSMinimapObstacleComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "Minimap/LSMinimapSubsystem.h"
#include "Vision/LSVisionCollisionGeometry.h"
#include "Vision/LSVisionSettings.h"
#include "Vision/LSVisionSubsystem.h"

ULSMinimapObstacleComponent::ULSMinimapObstacleComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULSMinimapObstacleComponent::AddTargetPrimitive(UPrimitiveComponent* Primitive)
{
	if (!IsValid(Primitive))
	{
		return;
	}

	TargetPrimitives.AddUnique(Primitive);
	if (IsRegistered())
	{
		RebuildSegments();
		UpdateObservedComponentBindings();
	}
}

void ULSMinimapObstacleComponent::OnRegister()
{
	Super::OnRegister();

	RebuildSegments();
	UpdateObservedComponentBindings();
}

void ULSMinimapObstacleComponent::BeginPlay()
{
	Super::BeginPlay();
	RebuildSegments();
	UpdateObservedComponentBindings();

	if (UWorld* World = GetWorld())
	{
		if (ULSMinimapSubsystem* MinimapSubsystem = World->GetSubsystem<ULSMinimapSubsystem>())
		{
			MinimapSubsystem->RegisterObstacle(this);
		}
	}
}

void ULSMinimapObstacleComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (ULSMinimapSubsystem* MinimapSubsystem = World->GetSubsystem<ULSMinimapSubsystem>())
		{
			MinimapSubsystem->UnregisterObstacle(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void ULSMinimapObstacleComponent::OnUnregister()
{
	ClearObservedComponentBindings();

	Super::OnUnregister();
}

void ULSMinimapObstacleComponent::GatherObstacleSegments(TArray<FLSVisionSegment2D>& OutSegments)
{
	OutSegments.Reset();
	if (!bVisibleOnMinimap)
	{
		return;
	}

	const float SliceZ = ResolveSliceZ();
	if (!FMath::IsNearlyEqual(CachedSliceZ, SliceZ, 1.0f))
	{
		RebuildSegments();
	}

	OutSegments = CachedSegments;
}

void ULSMinimapObstacleComponent::RebuildSegments()
{
	CachedSegments.Reset();
	CachedSliceZ = ResolveSliceZ();

	TArray<UPrimitiveComponent*> SourcePrimitives;
	GatherSourcePrimitives(SourcePrimitives);
	for (const UPrimitiveComponent* Primitive : SourcePrimitives)
	{
		LSVisionCollisionGeometry::AppendCollisionSegments(Primitive, CachedSliceZ, CachedSegments);
	}
}

void ULSMinimapObstacleComponent::GatherSourcePrimitives(TArray<UPrimitiveComponent*>& OutPrimitives) const
{
	OutPrimitives.Reset();
	for (UPrimitiveComponent* Primitive : TargetPrimitives)
	{
		if (ShouldUsePrimitive(Primitive))
		{
			OutPrimitives.AddUnique(Primitive);
		}
	}

	if (OutPrimitives.Num() > 0 || !bUseOwnerBlockingPrimitives || GetOwner() == nullptr)
	{
		return;
	}

	TArray<UPrimitiveComponent*> OwnerPrimitives;
	GetOwner()->GetComponents<UPrimitiveComponent>(OwnerPrimitives);
	for (UPrimitiveComponent* Primitive : OwnerPrimitives)
	{
		if (ShouldUsePrimitive(Primitive))
		{
			OutPrimitives.AddUnique(Primitive);
		}
	}
}

void ULSMinimapObstacleComponent::ClearObservedComponentBindings()
{
	for (const TWeakObjectPtr<USceneComponent>& SceneComponent : ObservedSceneComponents)
	{
		if (SceneComponent.IsValid())
		{
			SceneComponent->TransformUpdated.RemoveAll(this);
		}
	}
	ObservedSceneComponents.Reset();
}

void ULSMinimapObstacleComponent::UpdateObservedComponentBindings()
{
	ClearObservedComponentBindings();

	TArray<UPrimitiveComponent*> SourcePrimitives;
	GatherSourcePrimitives(SourcePrimitives);
	for (UPrimitiveComponent* Primitive : SourcePrimitives)
	{
		Primitive->TransformUpdated.AddUObject(this, &ULSMinimapObstacleComponent::HandleObservedComponentTransformUpdated);
		ObservedSceneComponents.AddUnique(Primitive);
	}
}

void ULSMinimapObstacleComponent::HandleObservedComponentTransformUpdated(
	USceneComponent* UpdatedComponent,
	EUpdateTransformFlags UpdateTransformFlags,
	ETeleportType Teleport)
{
	RebuildSegments();
}

float ULSMinimapObstacleComponent::ResolveSliceZ() const
{
	const ULSVisionSettings* VisionSettings = GetDefault<ULSVisionSettings>();
	if (VisionSettings != nullptr && VisionSettings->bSliceHeightFromPlayer)
	{
		if (const UWorld* World = GetWorld())
		{
			if (const ULSVisionSubsystem* VisionSubsystem = World->GetSubsystem<ULSVisionSubsystem>())
			{
				return VisionSubsystem->GetRuntimeSliceZ();
			}
		}
	}

	return VisionSettings != nullptr ? VisionSettings->OccluderSliceHeight : 0.0f;
}

bool ULSMinimapObstacleComponent::ShouldUsePrimitive(const UPrimitiveComponent* Primitive) const
{
	if (!IsValid(Primitive) || Primitive->GetCollisionEnabled() == ECollisionEnabled::NoCollision)
	{
		return false;
	}

	return Primitive->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block;
}
