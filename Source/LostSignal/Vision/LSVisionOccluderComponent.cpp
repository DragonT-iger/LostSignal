#include "Vision/LSVisionOccluderComponent.h"

#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "LostSignal.h"
#include "PhysicsEngine/BodySetup.h"
#include "Vision/LSVisionSubsystem.h"

ULSVisionOccluderComponent::ULSVisionOccluderComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
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
	case ELSVisionOccluderSourceMode::CollisionGeometry:
		if (UPrimitiveComponent* PrimitiveComponent = ResolveMeshPrimitiveComponent())
		{
			BuildSegmentsFromCollisionGeometry(PrimitiveComponent, RebuiltSegments);
		}
		break;

	case ELSVisionOccluderSourceMode::BoxComponent:
		if (UBoxComponent* BoxComponent = ResolveBoxComponent())
		{
			BuildSegmentsFromBox(BoxComponent, RebuiltSegments);
		}
		break;

	case ELSVisionOccluderSourceMode::MeshBounds:
		if (UPrimitiveComponent* PrimitiveComponent = ResolvePrimitiveComponent())
		{
			BuildSegmentsFromMeshBounds(PrimitiveComponent, RebuiltSegments);
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

void ULSVisionOccluderComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bDrawDebugSegments)
	{
		DrawDebugSegments();
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

// Builds 2D segments from simple collision geometry so holes/openings can be described by multiple primitives.
void ULSVisionOccluderComponent::BuildSegmentsFromCollisionGeometry(
	const UPrimitiveComponent* PrimitiveComponent,
	TArray<FLSVisionSegment2D>& OutSegments) const
{
	if (PrimitiveComponent == nullptr)
	{
		return;
	}

	const UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(PrimitiveComponent);
	const UStaticMesh* StaticMesh = StaticMeshComponent != nullptr ? StaticMeshComponent->GetStaticMesh() : nullptr;
	const UBodySetup* BodySetup = StaticMesh != nullptr ? StaticMesh->GetBodySetup() : nullptr;
	if (BodySetup == nullptr)
	{
		BuildSegmentsFromMeshBounds(PrimitiveComponent, OutSegments);
		return;
	}

	const FTransform ComponentTransform = PrimitiveComponent->GetComponentTransform();
	const FKAggregateGeom& AggregateGeometry = BodySetup->AggGeom;

	for (const FKBoxElem& BoxElement : AggregateGeometry.BoxElems)
	{
		const FVector HalfExtent(BoxElement.X * 0.5f, BoxElement.Y * 0.5f, BoxElement.Z * 0.5f);
		const FTransform BoxLocalTransform = BoxElement.GetTransform();

		const FVector World0 = ComponentTransform.TransformPosition(BoxLocalTransform.TransformPosition(FVector(HalfExtent.X, HalfExtent.Y, 0.0f)));
		const FVector World1 = ComponentTransform.TransformPosition(BoxLocalTransform.TransformPosition(FVector(HalfExtent.X, -HalfExtent.Y, 0.0f)));
		const FVector World2 = ComponentTransform.TransformPosition(BoxLocalTransform.TransformPosition(FVector(-HalfExtent.X, -HalfExtent.Y, 0.0f)));
		const FVector World3 = ComponentTransform.TransformPosition(BoxLocalTransform.TransformPosition(FVector(-HalfExtent.X, HalfExtent.Y, 0.0f)));

		TArray<FVector2D> BoxLoop;
		BoxLoop.Reserve(4);
		BoxLoop.Add(FVector2D(World0.X, World0.Y));
		BoxLoop.Add(FVector2D(World1.X, World1.Y));
		BoxLoop.Add(FVector2D(World2.X, World2.Y));
		BoxLoop.Add(FVector2D(World3.X, World3.Y));
		AddClosedPointLoopSegments(BoxLoop, OutSegments);
	}

	for (const FKConvexElem& ConvexElement : AggregateGeometry.ConvexElems)
	{
		if (ConvexElement.VertexData.Num() < 3)
		{
			continue;
		}

		TArray<FVector2D> ProjectedPoints;
		ProjectedPoints.Reserve(ConvexElement.VertexData.Num());

		const FTransform ConvexLocalTransform = ConvexElement.GetTransform();
		for (const FVector& LocalVertex : ConvexElement.VertexData)
		{
			const FVector WorldVertex = ComponentTransform.TransformPosition(ConvexLocalTransform.TransformPosition(LocalVertex));
			ProjectedPoints.Add(FVector2D(WorldVertex.X, WorldVertex.Y));
		}

		TArray<FVector2D> HullPoints;
		BuildConvexHull2D(ProjectedPoints, HullPoints);
		AddClosedPointLoopSegments(HullPoints, OutSegments);
	}

	if (OutSegments.Num() == 0)
	{
		BuildSegmentsFromMeshBounds(PrimitiveComponent, OutSegments);
	}
}

// Builds 2D segments from the owning mesh's local bounds so rotation/pivot are preserved better than a world AABB.
void ULSVisionOccluderComponent::BuildSegmentsFromMeshBounds(
	const UPrimitiveComponent* PrimitiveComponent,
	TArray<FLSVisionSegment2D>& OutSegments) const
{
	if (PrimitiveComponent == nullptr)
	{
		return;
	}

	if (const UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(PrimitiveComponent))
	{
		FVector LocalMin = FVector::ZeroVector;
		FVector LocalMax = FVector::ZeroVector;
		StaticMeshComponent->GetLocalBounds(LocalMin, LocalMax);

		const FVector LocalCenter = (LocalMin + LocalMax) * 0.5f;
		AddTransformedRectangleSegments(
			LocalCenter,
			FVector2D(LocalMin.X, LocalMin.Y),
			FVector2D(LocalMax.X, LocalMax.Y),
			StaticMeshComponent->GetComponentTransform(),
			OutSegments);
		return;
	}

	BuildSegmentsFromPrimitiveBounds(PrimitiveComponent, OutSegments);
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

// Emits four line segments from a local-space rectangle after applying the component transform.
void ULSVisionOccluderComponent::AddTransformedRectangleSegments(
	const FVector& LocalCenter,
	const FVector2D& LocalMin,
	const FVector2D& LocalMax,
	const FTransform& LocalToWorld,
	TArray<FLSVisionSegment2D>& OutSegments) const
{
	const FVector World0 = LocalToWorld.TransformPosition(FVector(LocalMax.X, LocalMax.Y, LocalCenter.Z));
	const FVector World1 = LocalToWorld.TransformPosition(FVector(LocalMax.X, LocalMin.Y, LocalCenter.Z));
	const FVector World2 = LocalToWorld.TransformPosition(FVector(LocalMin.X, LocalMin.Y, LocalCenter.Z));
	const FVector World3 = LocalToWorld.TransformPosition(FVector(LocalMin.X, LocalMax.Y, LocalCenter.Z));

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

// Emits a segment per edge in a closed 2D loop, skipping zero-length edges.
void ULSVisionOccluderComponent::AddClosedPointLoopSegments(
	const TArray<FVector2D>& Points,
	TArray<FLSVisionSegment2D>& OutSegments) const
{
	if (Points.Num() < 2)
	{
		return;
	}

	for (int32 PointIndex = 0; PointIndex < Points.Num(); ++PointIndex)
	{
		const FVector2D& StartPoint = Points[PointIndex];
		const FVector2D& EndPoint = Points[(PointIndex + 1) % Points.Num()];

		if (StartPoint.Equals(EndPoint, KINDA_SMALL_NUMBER))
		{
			continue;
		}

		FLSVisionSegment2D Segment;
		Segment.Start = StartPoint;
		Segment.End = EndPoint;
		OutSegments.Add(Segment);
	}
}

// Builds a 2D convex hull from projected collision vertices so convex simple collision maps cleanly to segments.
void ULSVisionOccluderComponent::BuildConvexHull2D(const TArray<FVector2D>& InputPoints, TArray<FVector2D>& OutHullPoints) const
{
	OutHullPoints.Reset();
	if (InputPoints.Num() < 3)
	{
		OutHullPoints = InputPoints;
		return;
	}

	TArray<FVector2D> UniquePoints;
	UniquePoints.Reserve(InputPoints.Num());

	for (const FVector2D& Point : InputPoints)
	{
		const bool bAlreadyAdded = UniquePoints.ContainsByPredicate(
			[&Point](const FVector2D& ExistingPoint)
			{
				return ExistingPoint.Equals(Point, KINDA_SMALL_NUMBER);
			});

		if (!bAlreadyAdded)
		{
			UniquePoints.Add(Point);
		}
	}

	if (UniquePoints.Num() < 3)
	{
		OutHullPoints = UniquePoints;
		return;
	}

	UniquePoints.Sort([](const FVector2D& A, const FVector2D& B)
	{
		if (!FMath::IsNearlyEqual(A.X, B.X))
		{
			return A.X < B.X;
		}

		return A.Y < B.Y;
	});

	const auto Cross = [](const FVector2D& A, const FVector2D& B, const FVector2D& C)
	{
		const FVector2D AB = B - A;
		const FVector2D AC = C - A;
		return (AB.X * AC.Y) - (AB.Y * AC.X);
	};

	TArray<FVector2D> LowerHull;
	for (const FVector2D& Point : UniquePoints)
	{
		while (LowerHull.Num() >= 2 && Cross(LowerHull[LowerHull.Num() - 2], LowerHull[LowerHull.Num() - 1], Point) <= 0.0f)
		{
			LowerHull.Pop();
		}

		LowerHull.Add(Point);
	}

	TArray<FVector2D> UpperHull;
	for (int32 PointIndex = UniquePoints.Num() - 1; PointIndex >= 0; --PointIndex)
	{
		const FVector2D& Point = UniquePoints[PointIndex];
		while (UpperHull.Num() >= 2 && Cross(UpperHull[UpperHull.Num() - 2], UpperHull[UpperHull.Num() - 1], Point) <= 0.0f)
		{
			UpperHull.Pop();
		}

		UpperHull.Add(Point);
	}

	LowerHull.Pop();
	UpperHull.Pop();

	OutHullPoints = MoveTemp(LowerHull);
	OutHullPoints.Append(UpperHull);
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

// Resolves the best mesh-like primitive source, preferring static mesh components over generic primitives.
UPrimitiveComponent* ULSVisionOccluderComponent::ResolveMeshPrimitiveComponent() const
{
	if (SourcePrimitiveComponent != nullptr)
	{
		return SourcePrimitiveComponent;
	}

	if (bAutoFindOwnerComponents && GetOwner() != nullptr)
	{
		if (UStaticMeshComponent* StaticMeshComponent = GetOwner()->FindComponentByClass<UStaticMeshComponent>())
		{
			return StaticMeshComponent;
		}

		return GetOwner()->FindComponentByClass<UPrimitiveComponent>();
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

// Renders the cached 2D blocking segments as world-space debug lines for quick authoring checks.
void ULSVisionOccluderComponent::DrawDebugSegments() const
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	for (const FLSVisionSegment2D& Segment : Segments)
	{
		const FVector Start(Segment.Start.X, Segment.Start.Y, DebugDrawZOffset);
		const FVector End(Segment.End.X, Segment.End.Y, DebugDrawZOffset);

		DrawDebugLine(
			World,
			Start,
			End,
			DebugSegmentColor,
			false,
			0.0f,
			0,
			DebugDrawThickness);
	}
}

// Returns the scene component whose transform should trigger a segment rebuild for this occluder.
USceneComponent* ULSVisionOccluderComponent::ResolveObservedSceneComponent() const
{
	switch (SourceMode)
	{
	case ELSVisionOccluderSourceMode::BoxComponent:
		return ResolveBoxComponent();

	case ELSVisionOccluderSourceMode::CollisionGeometry:
		return ResolveMeshPrimitiveComponent();

	case ELSVisionOccluderSourceMode::MeshBounds:
		return ResolveMeshPrimitiveComponent();

	case ELSVisionOccluderSourceMode::PrimitiveBounds:
		return ResolvePrimitiveComponent();

	case ELSVisionOccluderSourceMode::ManualSegments:
	default:
		return nullptr;
	}
}
