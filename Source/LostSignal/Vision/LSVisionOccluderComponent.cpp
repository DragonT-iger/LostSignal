#include "Vision/LSVisionOccluderComponent.h"

#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"
#include "LostSignal.h"
#include "Vision/LSVisionSubsystem.h"

ULSVisionOccluderComponent::ULSVisionOccluderComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// Prepares the initial segment cache and optional transform observer as soon as the component is registered.
void ULSVisionOccluderComponent::OnRegister()
{
	Super::OnRegister();

	RebuildSegments();
	UpdateObservedComponentBinding();
}

// Rebuilds once more at runtime and registers the component with the world vision subsystem.
void ULSVisionOccluderComponent::BeginPlay()
{
	Super::BeginPlay();
	RebuildSegments();
	UpdateObservedComponentBinding();

	if (UWorld* World = GetWorld())
	{
		if (ULSVisionSubsystem* VisionSubsystem = World->GetSubsystem<ULSVisionSubsystem>())
		{
			VisionSubsystem->RegisterOccluder(this);
		}
	}
}

// Unregisters this occluder so stale segment data is not used after teardown.
void ULSVisionOccluderComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (ULSVisionSubsystem* VisionSubsystem = World->GetSubsystem<ULSVisionSubsystem>())
		{
			VisionSubsystem->UnregisterOccluder(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

// Clears transform listeners when the component is being removed from the actor/component tree.
void ULSVisionOccluderComponent::OnUnregister()
{
	UpdateObservedComponentBinding();

	Super::OnUnregister();
}

// Rebuilds the occluder segment cache from the currently selected source mode.
void ULSVisionOccluderComponent::RebuildSegments()
{
	TArray<FLSVisionSegment2D> RebuiltSegments;

	switch (SourceMode)
	{
	case ELSVisionOccluderSourceMode::BoxComponent:
		if (UBoxComponent* BoxComponent = ResolveBoxComponent())
		{
			BuildSegmentsFromBox(BoxComponent, RebuiltSegments);
		}
		break;

	case ELSVisionOccluderSourceMode::PrimitiveBounds:
		if (UPrimitiveComponent* PrimitiveComponent = ResolvePrimitiveComponent())
		{
			BuildSegmentsFromPrimitiveBounds(PrimitiveComponent, RebuiltSegments);
		}
		break;

	case ELSVisionOccluderSourceMode::ManualSegments:
		RebuiltSegments = ManualSegments;
		break;

	default:
		break;
	}

	Segments = MoveTemp(RebuiltSegments);

	if (Segments.Num() == 0 && GetOwner() != nullptr)
	{
		UE_LOG(LogLS, Warning, TEXT("LSVisionOccluderComponent on '%s' could not build any occluder segments."), *GetNameSafe(GetOwner()));
	}
}

// Keeps transform-change rebuilding attached only to the scene component that actually drives this occluder.
void ULSVisionOccluderComponent::UpdateObservedComponentBinding()
{
	if (ObservedSceneComponent.IsValid())
	{
		ObservedSceneComponent->TransformUpdated.RemoveAll(this);
		ObservedSceneComponent.Reset();
	}

	if (!bRebuildOnTransformChanged || SourceMode == ELSVisionOccluderSourceMode::ManualSegments)
	{
		return;
	}

	if (USceneComponent* SceneComponent = ResolveObservedSceneComponent())
	{
		SceneComponent->TransformUpdated.AddUObject(this, &ULSVisionOccluderComponent::HandleObservedComponentTransformUpdated);
		ObservedSceneComponent = SceneComponent;
	}
}

// Refreshes the cached 2D segments whenever the observed source component moves.
void ULSVisionOccluderComponent::HandleObservedComponentTransformUpdated(
	USceneComponent* UpdatedComponent,
	EUpdateTransformFlags UpdateTransformFlags,
	ETeleportType Teleport)
{
	RebuildSegments();
}

// Converts a box component into four 2D blocking edges in world space.
void ULSVisionOccluderComponent::BuildSegmentsFromBox(const UBoxComponent* BoxComponent, TArray<FLSVisionSegment2D>& OutSegments) const
{
	if (BoxComponent == nullptr)
	{
		return;
	}

	const FVector Extent = BoxComponent->GetUnscaledBoxExtent();
	const FTransform BoxTransform = BoxComponent->GetComponentTransform();

	const FVector World0 = BoxTransform.TransformPosition(FVector(Extent.X, Extent.Y, 0.0f));
	const FVector World1 = BoxTransform.TransformPosition(FVector(Extent.X, -Extent.Y, 0.0f));
	const FVector World2 = BoxTransform.TransformPosition(FVector(-Extent.X, -Extent.Y, 0.0f));
	const FVector World3 = BoxTransform.TransformPosition(FVector(-Extent.X, Extent.Y, 0.0f));

	FLSVisionSegment2D Segment01;
	Segment01.Start = FVector2D(World0.X, World0.Y);
	Segment01.End = FVector2D(World1.X, World1.Y);
	OutSegments.Add(Segment01);

	FLSVisionSegment2D Segment12;
	Segment12.Start = FVector2D(World1.X, World1.Y);
	Segment12.End = FVector2D(World2.X, World2.Y);
	OutSegments.Add(Segment12);

	FLSVisionSegment2D Segment23;
	Segment23.Start = FVector2D(World2.X, World2.Y);
	Segment23.End = FVector2D(World3.X, World3.Y);
	OutSegments.Add(Segment23);

	FLSVisionSegment2D Segment30;
	Segment30.Start = FVector2D(World3.X, World3.Y);
	Segment30.End = FVector2D(World0.X, World0.Y);
	OutSegments.Add(Segment30);
}

// Approximates an arbitrary primitive as a 2D bounds rectangle for easy environment authoring.
void ULSVisionOccluderComponent::BuildSegmentsFromPrimitiveBounds(
	const UPrimitiveComponent* PrimitiveComponent,
	TArray<FLSVisionSegment2D>& OutSegments) const
{
	if (PrimitiveComponent == nullptr)
	{
		return;
	}

	const FBoxSphereBounds Bounds = PrimitiveComponent->Bounds;
	const FVector Extent = Bounds.BoxExtent;
	const FVector Origin = Bounds.Origin;

	const FVector2D Min(Origin.X - Extent.X, Origin.Y - Extent.Y);
	const FVector2D Max(Origin.X + Extent.X, Origin.Y + Extent.Y);
	AddRectangleSegments(Min, Max, OutSegments);
}

// Emits four line segments from a 2D AABB, used by both bounds-based occluder sources.
void ULSVisionOccluderComponent::AddRectangleSegments(
	const FVector2D& Min,
	const FVector2D& Max,
	TArray<FLSVisionSegment2D>& OutSegments) const
{
	FLSVisionSegment2D SegmentBottom;
	SegmentBottom.Start = FVector2D(Max.X, Max.Y);
	SegmentBottom.End = FVector2D(Max.X, Min.Y);
	OutSegments.Add(SegmentBottom);

	FLSVisionSegment2D SegmentLeft;
	SegmentLeft.Start = FVector2D(Max.X, Min.Y);
	SegmentLeft.End = FVector2D(Min.X, Min.Y);
	OutSegments.Add(SegmentLeft);

	FLSVisionSegment2D SegmentTop;
	SegmentTop.Start = FVector2D(Min.X, Min.Y);
	SegmentTop.End = FVector2D(Min.X, Max.Y);
	OutSegments.Add(SegmentTop);

	FLSVisionSegment2D SegmentRight;
	SegmentRight.Start = FVector2D(Min.X, Max.Y);
	SegmentRight.End = FVector2D(Max.X, Max.Y);
	OutSegments.Add(SegmentRight);
}

// Resolves the box source explicitly first, then falls back to the owner's first box component.
UBoxComponent* ULSVisionOccluderComponent::ResolveBoxComponent() const
{
	if (SourceBoxComponent != nullptr)
	{
		return SourceBoxComponent;
	}

	if (bAutoFindOwnerComponents && GetOwner() != nullptr)
	{
		return GetOwner()->FindComponentByClass<UBoxComponent>();
	}

	return nullptr;
}

// Resolves the primitive source explicitly first, then falls back to the owner's first primitive component.
UPrimitiveComponent* ULSVisionOccluderComponent::ResolvePrimitiveComponent() const
{
	if (SourcePrimitiveComponent != nullptr)
	{
		return SourcePrimitiveComponent;
	}

	if (bAutoFindOwnerComponents && GetOwner() != nullptr)
	{
		return GetOwner()->FindComponentByClass<UPrimitiveComponent>();
	}

	return nullptr;
}

// Returns the scene component whose transform should trigger a segment rebuild for this occluder.
USceneComponent* ULSVisionOccluderComponent::ResolveObservedSceneComponent() const
{
	switch (SourceMode)
	{
	case ELSVisionOccluderSourceMode::BoxComponent:
		return ResolveBoxComponent();

	case ELSVisionOccluderSourceMode::PrimitiveBounds:
		return ResolvePrimitiveComponent();

	case ELSVisionOccluderSourceMode::ManualSegments:
	default:
		return nullptr;
	}
}
