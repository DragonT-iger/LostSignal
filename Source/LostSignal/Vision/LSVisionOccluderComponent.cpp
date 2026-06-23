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
#include "Vision/LSVisionSettings.h"
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

	const float SliceZ = ResolveSliceZ();

	switch (SourceMode)
	{
	case ELSVisionOccluderSourceMode::CollisionGeometry:
		if (UPrimitiveComponent* PrimitiveComponent = ResolveMeshPrimitiveComponent())
		{
			BuildSegmentsFromCollisionGeometry(PrimitiveComponent, SliceZ, RebuiltSegments);
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
			BuildSegmentsFromMeshBounds(PrimitiveComponent, SliceZ, RebuiltSegments);
		}
		break;

	case ELSVisionOccluderSourceMode::PrimitiveBounds:
		if (UPrimitiveComponent* PrimitiveComponent = ResolvePrimitiveComponent())
		{
			BuildSegmentsFromPrimitiveBounds(PrimitiveComponent, SliceZ, RebuiltSegments);
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
		// 시야 평면(SliceZ)에 형상이 없으면(떠 있는/빈 바닥 물체) 세그먼트가 0인 게 정상이라 경고 대신 Verbose로 남긴다.
		UE_LOG(LogLS, Verbose, TEXT("LSVisionOccluderComponent on '%s' produced no occluder segments (collision may not cross the vision slice height)."), *GetNameSafe(GetOwner()));
	}

	if (GetOwner() != nullptr && GetOwner()->HasActorBegunPlay())
	{
		if (UWorld* World = GetWorld())
		{
			if (ULSVisionSubsystem* VisionSubsystem = World->GetSubsystem<ULSVisionSubsystem>())
			{
				VisionSubsystem->RefreshOccluder(this);
			}
		}
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

// Builds 2D segments from the cross-section of simple collision at the vision slice height (Z=SliceZ),
// so floating/empty-bottom or tilted objects only block sight where geometry actually exists at floor level.
void ULSVisionOccluderComponent::BuildSegmentsFromCollisionGeometry(
	const UPrimitiveComponent* PrimitiveComponent,
	const float SliceZ,
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
		BuildSegmentsFromMeshBounds(PrimitiveComponent, SliceZ, OutSegments);
		return;
	}

	const FTransform ComponentTransform = PrimitiveComponent->GetComponentTransform();
	const FKAggregateGeom& AggregateGeometry = BodySetup->AggGeom;

	const int32 ShapeCount = AggregateGeometry.BoxElems.Num() + AggregateGeometry.ConvexElems.Num();

	for (const FKBoxElem& BoxElement : AggregateGeometry.BoxElems)
	{
		SliceBoxByPlane(ComponentTransform, BoxElement, SliceZ, OutSegments);
	}

	for (const FKConvexElem& ConvexElement : AggregateGeometry.ConvexElems)
	{
		SliceConvexByPlane(ComponentTransform, ConvexElement, SliceZ, OutSegments);
	}

	// 박스/콘벡스가 아예 없으면(스피어/캡슐 등) 바운드로 폴백. 형상은 있는데 SliceZ를 안 지나 비어있는 건 정상이므로 폴백 안 함.
	if (ShapeCount == 0)
	{
		BuildSegmentsFromMeshBounds(PrimitiveComponent, SliceZ, OutSegments);
	}
}

// Clips an oriented box's 12 edges against the horizontal plane Z=SliceZ and turns the cross-section into segments.
void ULSVisionOccluderComponent::SliceBoxByPlane(
	const FTransform& ComponentTransform,
	const FKBoxElem& BoxElement,
	const float SliceZ,
	TArray<FLSVisionSegment2D>& OutSegments) const
{
	const FVector HalfExtent(BoxElement.X * 0.5f, BoxElement.Y * 0.5f, BoxElement.Z * 0.5f);
	const FTransform BoxLocalTransform = BoxElement.GetTransform();

	// 코너 순서: (sx,sy,sz) 0:(-,-,-) 1:(-,-,+) 2:(-,+,-) 3:(-,+,+) 4:(+,-,-) 5:(+,-,+) 6:(+,+,-) 7:(+,+,+)
	FVector Corners[8];
	int32 CornerIndex = 0;
	for (int32 SignX = -1; SignX <= 1; SignX += 2)
	{
		for (int32 SignY = -1; SignY <= 1; SignY += 2)
		{
			for (int32 SignZ = -1; SignZ <= 1; SignZ += 2)
			{
				const FVector LocalCorner(SignX * HalfExtent.X, SignY * HalfExtent.Y, SignZ * HalfExtent.Z);
				Corners[CornerIndex++] = ComponentTransform.TransformPosition(BoxLocalTransform.TransformPosition(LocalCorner));
			}
		}
	}

	static const int32 BoxEdges[12][2] =
	{
		{0, 1}, {2, 3}, {4, 5}, {6, 7}, // sz 방향
		{0, 2}, {1, 3}, {4, 6}, {5, 7}, // sy 방향
		{0, 4}, {1, 5}, {2, 6}, {3, 7}  // sx 방향
	};

	TArray<FVector2D> SlicePoints;
	for (const int32(&Edge)[2] : BoxEdges)
	{
		const FVector& A = Corners[Edge[0]];
		const FVector& B = Corners[Edge[1]];
		const float DistA = A.Z - SliceZ;
		const float DistB = B.Z - SliceZ;

		if ((DistA <= 0.0f && DistB >= 0.0f) || (DistA >= 0.0f && DistB <= 0.0f))
		{
			if (FMath::IsNearlyEqual(A.Z, B.Z))
			{
				continue;
			}

			const float T = FMath::Clamp((SliceZ - A.Z) / (B.Z - A.Z), 0.0f, 1.0f);
			const FVector P = FMath::Lerp(A, B, T);
			SlicePoints.Add(FVector2D(P.X, P.Y));
		}
	}

	if (SlicePoints.Num() >= 3)
	{
		TArray<FVector2D> HullPoints;
		BuildConvexHull2D(SlicePoints, HullPoints);
		AddClosedPointLoopSegments(HullPoints, OutSegments);
	}
}

// Clips a convex hull's edges against the horizontal plane Z=SliceZ and turns the cross-section into segments.
void ULSVisionOccluderComponent::SliceConvexByPlane(
	const FTransform& ComponentTransform,
	const FKConvexElem& ConvexElement,
	const float SliceZ,
	TArray<FLSVisionSegment2D>& OutSegments) const
{
	const TArray<FVector>& VertexData = ConvexElement.VertexData;
	if (VertexData.Num() < 4)
	{
		return;
	}

	const FTransform ConvexLocalTransform = ConvexElement.GetTransform();
	TArray<FVector> WorldVertices;
	WorldVertices.Reserve(VertexData.Num());
	for (const FVector& LocalVertex : VertexData)
	{
		WorldVertices.Add(ComponentTransform.TransformPosition(ConvexLocalTransform.TransformPosition(LocalVertex)));
	}

	TArray<FVector2D> SlicePoints;

	const TArray<int32>& IndexData = ConvexElement.IndexData;
	if (IndexData.Num() >= 3)
	{
		TSet<uint32> VisitedEdges;
		const auto SliceEdge = [&](const int32 IndexA, const int32 IndexB)
		{
			if (IndexA == IndexB || !WorldVertices.IsValidIndex(IndexA) || !WorldVertices.IsValidIndex(IndexB))
			{
				return;
			}

			const uint32 EdgeKey = IndexA < IndexB
				? (static_cast<uint32>(IndexA) << 16) | static_cast<uint32>(IndexB)
				: (static_cast<uint32>(IndexB) << 16) | static_cast<uint32>(IndexA);
			bool bAlreadyVisited = false;
			VisitedEdges.Add(EdgeKey, &bAlreadyVisited);
			if (bAlreadyVisited)
			{
				return;
			}

			const FVector& A = WorldVertices[IndexA];
			const FVector& B = WorldVertices[IndexB];
			const float DistA = A.Z - SliceZ;
			const float DistB = B.Z - SliceZ;
			if (((DistA <= 0.0f && DistB >= 0.0f) || (DistA >= 0.0f && DistB <= 0.0f)) && !FMath::IsNearlyEqual(A.Z, B.Z))
			{
				const float T = FMath::Clamp((SliceZ - A.Z) / (B.Z - A.Z), 0.0f, 1.0f);
				const FVector P = FMath::Lerp(A, B, T);
				SlicePoints.Add(FVector2D(P.X, P.Y));
			}
		};

		for (int32 TriangleStart = 0; TriangleStart + 2 < IndexData.Num(); TriangleStart += 3)
		{
			SliceEdge(IndexData[TriangleStart], IndexData[TriangleStart + 1]);
			SliceEdge(IndexData[TriangleStart + 1], IndexData[TriangleStart + 2]);
			SliceEdge(IndexData[TriangleStart + 2], IndexData[TriangleStart]);
		}
	}
	else
	{
		// 인덱스(엣지) 데이터가 없으면 정밀 절단이 불가능하므로, 기존처럼 전체 정점을 투영해 폴백한다.
		for (const FVector& WorldVertex : WorldVertices)
		{
			SlicePoints.Add(FVector2D(WorldVertex.X, WorldVertex.Y));
		}
	}

	if (SlicePoints.Num() >= 3)
	{
		TArray<FVector2D> HullPoints;
		BuildConvexHull2D(SlicePoints, HullPoints);
		AddClosedPointLoopSegments(HullPoints, OutSegments);
	}
}

// Returns true when the vision slice plane (Z=SliceZ) lies within the primitive's world bounds Z range.
namespace
{
	bool DoesSlicePlaneOverlapBounds(const UPrimitiveComponent* PrimitiveComponent, const float SliceZ)
	{
		const FBoxSphereBounds WorldBounds = PrimitiveComponent->Bounds;
		const float MinZ = WorldBounds.Origin.Z - WorldBounds.BoxExtent.Z;
		const float MaxZ = WorldBounds.Origin.Z + WorldBounds.BoxExtent.Z;
		return SliceZ >= MinZ && SliceZ <= MaxZ;
	}
}

// Builds 2D segments from the owning mesh's local bounds so rotation/pivot are preserved better than a world AABB.
void ULSVisionOccluderComponent::BuildSegmentsFromMeshBounds(
	const UPrimitiveComponent* PrimitiveComponent,
	const float SliceZ,
	TArray<FLSVisionSegment2D>& OutSegments) const
{
	if (PrimitiveComponent == nullptr)
	{
		return;
	}

	// 바운드 AABB는 어느 높이로 잘라도 같은 사각형이므로, SliceZ가 바운드 Z범위 밖이면(떠 있는 물체) 막지 않는다.
	if (!DoesSlicePlaneOverlapBounds(PrimitiveComponent, SliceZ))
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

	BuildSegmentsFromPrimitiveBounds(PrimitiveComponent, SliceZ, OutSegments);
}

// Approximates an arbitrary primitive as a 2D bounds rectangle for easy environment authoring.
void ULSVisionOccluderComponent::BuildSegmentsFromPrimitiveBounds(
	const UPrimitiveComponent* PrimitiveComponent,
	const float SliceZ,
	TArray<FLSVisionSegment2D>& OutSegments) const
{
	if (PrimitiveComponent == nullptr)
	{
		return;
	}

	// 바운드 AABB는 어느 높이로 잘라도 같은 사각형이므로, SliceZ가 바운드 Z범위 밖이면(떠 있는 물체) 막지 않는다.
	if (!DoesSlicePlaneOverlapBounds(PrimitiveComponent, SliceZ))
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

// 콜리전을 자를 시야 평면 높이(월드 Z).
// 플레이어 발 높이 기준 모드면 서브시스템이 런타임에 갱신한 값을, 아니면 설정의 절대 Z를 쓴다.
float ULSVisionOccluderComponent::ResolveSliceZ() const
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
