#include "Vision/LSVisionCollisionGeometry.h"

#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/BodySetup.h"

namespace
{
bool DoesCollisionGeometrySliceOverlapBounds(const UPrimitiveComponent* PrimitiveComponent, const float SliceZ)
{
	const FBoxSphereBounds WorldBounds = PrimitiveComponent->Bounds;
	const float MinZ = WorldBounds.Origin.Z - WorldBounds.BoxExtent.Z;
	const float MaxZ = WorldBounds.Origin.Z + WorldBounds.BoxExtent.Z;
	return SliceZ >= MinZ && SliceZ <= MaxZ;
}

void AppendCollisionGeometryClosedLoop(
	const TArray<FVector2D>& Points,
	TArray<FLSVisionSegment2D>& OutSegments)
{
	for (int32 PointIndex = 0; PointIndex < Points.Num(); ++PointIndex)
	{
		const FVector2D& StartPoint = Points[PointIndex];
		const FVector2D& EndPoint = Points[(PointIndex + 1) % Points.Num()];
		if (StartPoint.Equals(EndPoint, KINDA_SMALL_NUMBER))
		{
			continue;
		}

		FLSVisionSegment2D& Segment = OutSegments.AddDefaulted_GetRef();
		Segment.Start = StartPoint;
		Segment.End = EndPoint;
	}
}

TArray<FVector2D> BuildCollisionGeometryUniquePoints(const TArray<FVector2D>& InputPoints)
{
	TArray<FVector2D> UniquePoints;
	UniquePoints.Reserve(InputPoints.Num());
	for (const FVector2D& Point : InputPoints)
	{
		bool bDuplicate = false;
		for (const FVector2D& ExistingPoint : UniquePoints)
		{
			if (ExistingPoint.Equals(Point, KINDA_SMALL_NUMBER))
			{
				bDuplicate = true;
				break;
			}
		}

		if (!bDuplicate)
		{
			UniquePoints.Add(Point);
		}
	}
	return UniquePoints;
}

float CrossCollisionGeometryPoints(const FVector2D& A, const FVector2D& B, const FVector2D& C)
{
	const FVector2D AB = B - A;
	const FVector2D AC = C - A;
	return (AB.X * AC.Y) - (AB.Y * AC.X);
}

TArray<FVector2D> BuildCollisionGeometryHullHalf(const TArray<FVector2D>& Points, const bool bReverse)
{
	TArray<FVector2D> Hull;
	for (int32 IterationIndex = 0; IterationIndex < Points.Num(); ++IterationIndex)
	{
		const int32 PointIndex = bReverse ? Points.Num() - 1 - IterationIndex : IterationIndex;
		const FVector2D& Point = Points[PointIndex];
		while (Hull.Num() >= 2 &&
			CrossCollisionGeometryPoints(Hull[Hull.Num() - 2], Hull[Hull.Num() - 1], Point) <= 0.0f)
		{
			Hull.Pop();
		}
		Hull.Add(Point);
	}
	return Hull;
}

void BuildCollisionGeometryConvexHull(
	const TArray<FVector2D>& InputPoints,
	TArray<FVector2D>& OutHullPoints)
{
	TArray<FVector2D> UniquePoints = BuildCollisionGeometryUniquePoints(InputPoints);
	if (UniquePoints.Num() < 3)
	{
		OutHullPoints = MoveTemp(UniquePoints);
		return;
	}
	UniquePoints.Sort([](const FVector2D& A, const FVector2D& B)
	{
		return !FMath::IsNearlyEqual(A.X, B.X) ? A.X < B.X : A.Y < B.Y;
	});

	TArray<FVector2D> LowerHull = BuildCollisionGeometryHullHalf(UniquePoints, false);
	TArray<FVector2D> UpperHull = BuildCollisionGeometryHullHalf(UniquePoints, true);
	LowerHull.Pop();
	UpperHull.Pop();
	OutHullPoints = MoveTemp(LowerHull);
	OutHullPoints.Append(UpperHull);
}

void AppendCollisionGeometryRectangle(
	const FVector2D& Min,
	const FVector2D& Max,
	TArray<FLSVisionSegment2D>& OutSegments)
{
	AppendCollisionGeometryClosedLoop(
		{FVector2D(Max.X, Max.Y), FVector2D(Max.X, Min.Y), FVector2D(Min.X, Min.Y), FVector2D(Min.X, Max.Y)},
		OutSegments);
}

void AppendCollisionGeometryTransformedRectangle(
	const FVector& LocalCenter,
	const FVector2D& LocalMin,
	const FVector2D& LocalMax,
	const FTransform& LocalToWorld,
	TArray<FLSVisionSegment2D>& OutSegments)
{
	const FVector World0 = LocalToWorld.TransformPosition(FVector(LocalMax.X, LocalMax.Y, LocalCenter.Z));
	const FVector World1 = LocalToWorld.TransformPosition(FVector(LocalMax.X, LocalMin.Y, LocalCenter.Z));
	const FVector World2 = LocalToWorld.TransformPosition(FVector(LocalMin.X, LocalMin.Y, LocalCenter.Z));
	const FVector World3 = LocalToWorld.TransformPosition(FVector(LocalMin.X, LocalMax.Y, LocalCenter.Z));
	AppendCollisionGeometryClosedLoop(
		{
			FVector2D(World0.X, World0.Y),
			FVector2D(World1.X, World1.Y),
			FVector2D(World2.X, World2.Y),
			FVector2D(World3.X, World3.Y)
		},
		OutSegments);
}

void BuildCollisionGeometryBoxCorners(
	const FTransform& ComponentTransform,
	const FKBoxElem& BoxElement,
	FVector(&OutCorners)[8])
{
	const FVector HalfExtent(BoxElement.X * 0.5f, BoxElement.Y * 0.5f, BoxElement.Z * 0.5f);
	const FTransform BoxLocalTransform = BoxElement.GetTransform();

	int32 CornerIndex = 0;
	for (int32 SignX = -1; SignX <= 1; SignX += 2)
	{
		for (int32 SignY = -1; SignY <= 1; SignY += 2)
		{
			for (int32 SignZ = -1; SignZ <= 1; SignZ += 2)
			{
				const FVector LocalCorner(SignX * HalfExtent.X, SignY * HalfExtent.Y, SignZ * HalfExtent.Z);
				OutCorners[CornerIndex++] = ComponentTransform.TransformPosition(BoxLocalTransform.TransformPosition(LocalCorner));
			}
		}
	}
}

void GatherCollisionGeometryBoxSlicePoints(
	const FVector(&Corners)[8],
	const float SliceZ,
	TArray<FVector2D>& OutSlicePoints)
{
	static const int32 BoxEdges[12][2] =
	{
		{0, 1}, {2, 3}, {4, 5}, {6, 7},
		{0, 2}, {1, 3}, {4, 6}, {5, 7},
		{0, 4}, {1, 5}, {2, 6}, {3, 7}
	};

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
			const FVector Point = FMath::Lerp(A, B, T);
			OutSlicePoints.Add(FVector2D(Point.X, Point.Y));
		}
	}
}

void AppendCollisionGeometryBoxSlice(
	const FTransform& ComponentTransform,
	const FKBoxElem& BoxElement,
	const float SliceZ,
	TArray<FLSVisionSegment2D>& OutSegments)
{
	FVector Corners[8];
	BuildCollisionGeometryBoxCorners(ComponentTransform, BoxElement, Corners);
	TArray<FVector2D> SlicePoints;
	GatherCollisionGeometryBoxSlicePoints(Corners, SliceZ, SlicePoints);
	if (SlicePoints.Num() >= 3)
	{
		TArray<FVector2D> HullPoints;
		BuildCollisionGeometryConvexHull(SlicePoints, HullPoints);
		AppendCollisionGeometryClosedLoop(HullPoints, OutSegments);
	}
}

void AppendCollisionGeometryEdgeSlice(
	const TArray<FVector>& WorldVertices,
	const int32 IndexA,
	const int32 IndexB,
	const float SliceZ,
	TSet<uint32>& VisitedEdges,
	TArray<FVector2D>& OutSlicePoints)
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
	if (((DistA <= 0.0f && DistB >= 0.0f) || (DistA >= 0.0f && DistB <= 0.0f)) &&
		!FMath::IsNearlyEqual(A.Z, B.Z))
	{
		const float T = FMath::Clamp((SliceZ - A.Z) / (B.Z - A.Z), 0.0f, 1.0f);
		const FVector Point = FMath::Lerp(A, B, T);
		OutSlicePoints.Add(FVector2D(Point.X, Point.Y));
	}
}

void GatherCollisionGeometryIndexedSlicePoints(
	const TArray<FVector>& WorldVertices,
	const TArray<int32>& IndexData,
	const float SliceZ,
	TArray<FVector2D>& OutSlicePoints)
{
	TSet<uint32> VisitedEdges;
	for (int32 TriangleStart = 0; TriangleStart + 2 < IndexData.Num(); TriangleStart += 3)
	{
		AppendCollisionGeometryEdgeSlice(WorldVertices, IndexData[TriangleStart], IndexData[TriangleStart + 1], SliceZ, VisitedEdges, OutSlicePoints);
		AppendCollisionGeometryEdgeSlice(WorldVertices, IndexData[TriangleStart + 1], IndexData[TriangleStart + 2], SliceZ, VisitedEdges, OutSlicePoints);
		AppendCollisionGeometryEdgeSlice(WorldVertices, IndexData[TriangleStart + 2], IndexData[TriangleStart], SliceZ, VisitedEdges, OutSlicePoints);
	}
}

TArray<FVector> BuildCollisionGeometryWorldVertices(
	const FTransform& ComponentTransform,
	const FKConvexElem& ConvexElement)
{
	const FTransform ConvexLocalTransform = ConvexElement.GetTransform();
	TArray<FVector> WorldVertices;
	WorldVertices.Reserve(ConvexElement.VertexData.Num());
	for (const FVector& LocalVertex : ConvexElement.VertexData)
	{
		WorldVertices.Add(ComponentTransform.TransformPosition(ConvexLocalTransform.TransformPosition(LocalVertex)));
	}
	return WorldVertices;
}

void AppendCollisionGeometryConvexSlice(
	const FTransform& ComponentTransform,
	const FKConvexElem& ConvexElement,
	const float SliceZ,
	TArray<FLSVisionSegment2D>& OutSegments)
{
	if (ConvexElement.VertexData.Num() < 4)
	{
		return;
	}

	const TArray<FVector> WorldVertices = BuildCollisionGeometryWorldVertices(ComponentTransform, ConvexElement);
	TArray<FVector2D> SlicePoints;
	const TArray<int32>& IndexData = ConvexElement.IndexData;
	if (IndexData.Num() >= 3)
	{
		GatherCollisionGeometryIndexedSlicePoints(WorldVertices, IndexData, SliceZ, SlicePoints);
	}
	else
	{
		for (const FVector& WorldVertex : WorldVertices)
		{
			SlicePoints.Add(FVector2D(WorldVertex.X, WorldVertex.Y));
		}
	}

	if (SlicePoints.Num() >= 3)
	{
		TArray<FVector2D> HullPoints;
		BuildCollisionGeometryConvexHull(SlicePoints, HullPoints);
		AppendCollisionGeometryClosedLoop(HullPoints, OutSegments);
	}
}
}

void LSVisionCollisionGeometry::AppendBoxComponentSegments(
	const UBoxComponent* BoxComponent,
	TArray<FLSVisionSegment2D>& OutSegments)
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
	AppendCollisionGeometryClosedLoop(
		{
			FVector2D(World0.X, World0.Y),
			FVector2D(World1.X, World1.Y),
			FVector2D(World2.X, World2.Y),
			FVector2D(World3.X, World3.Y)
		},
		OutSegments);
}

void LSVisionCollisionGeometry::AppendCollisionSegments(
	const UPrimitiveComponent* PrimitiveComponent,
	const float SliceZ,
	TArray<FLSVisionSegment2D>& OutSegments)
{
	const UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(PrimitiveComponent);
	const UStaticMesh* StaticMesh = StaticMeshComponent != nullptr ? StaticMeshComponent->GetStaticMesh() : nullptr;
	const UBodySetup* BodySetup = StaticMesh != nullptr ? StaticMesh->GetBodySetup() : nullptr;
	if (BodySetup == nullptr)
	{
		AppendMeshBoundsSegments(PrimitiveComponent, SliceZ, OutSegments);
		return;
	}

	const FKAggregateGeom& AggregateGeometry = BodySetup->AggGeom;
	const int32 ShapeCount = AggregateGeometry.BoxElems.Num() + AggregateGeometry.ConvexElems.Num();
	for (const FKBoxElem& BoxElement : AggregateGeometry.BoxElems)
	{
		AppendCollisionGeometryBoxSlice(PrimitiveComponent->GetComponentTransform(), BoxElement, SliceZ, OutSegments);
	}
	for (const FKConvexElem& ConvexElement : AggregateGeometry.ConvexElems)
	{
		AppendCollisionGeometryConvexSlice(PrimitiveComponent->GetComponentTransform(), ConvexElement, SliceZ, OutSegments);
	}

	if (ShapeCount == 0)
	{
		AppendMeshBoundsSegments(PrimitiveComponent, SliceZ, OutSegments);
	}
}

void LSVisionCollisionGeometry::AppendMeshBoundsSegments(
	const UPrimitiveComponent* PrimitiveComponent,
	const float SliceZ,
	TArray<FLSVisionSegment2D>& OutSegments)
{
	if (PrimitiveComponent == nullptr || !DoesCollisionGeometrySliceOverlapBounds(PrimitiveComponent, SliceZ))
	{
		return;
	}

	if (const UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(PrimitiveComponent))
	{
		FVector LocalMin = FVector::ZeroVector;
		FVector LocalMax = FVector::ZeroVector;
		StaticMeshComponent->GetLocalBounds(LocalMin, LocalMax);
		AppendCollisionGeometryTransformedRectangle(
			(LocalMin + LocalMax) * 0.5f,
			FVector2D(LocalMin.X, LocalMin.Y),
			FVector2D(LocalMax.X, LocalMax.Y),
			StaticMeshComponent->GetComponentTransform(),
			OutSegments);
		return;
	}

	AppendPrimitiveBoundsSegments(PrimitiveComponent, SliceZ, OutSegments);
}

void LSVisionCollisionGeometry::AppendPrimitiveBoundsSegments(
	const UPrimitiveComponent* PrimitiveComponent,
	const float SliceZ,
	TArray<FLSVisionSegment2D>& OutSegments)
{
	if (PrimitiveComponent == nullptr || !DoesCollisionGeometrySliceOverlapBounds(PrimitiveComponent, SliceZ))
	{
		return;
	}

	const FBoxSphereBounds Bounds = PrimitiveComponent->Bounds;
	const FVector2D Min(Bounds.Origin.X - Bounds.BoxExtent.X, Bounds.Origin.Y - Bounds.BoxExtent.Y);
	const FVector2D Max(Bounds.Origin.X + Bounds.BoxExtent.X, Bounds.Origin.Y + Bounds.BoxExtent.Y);
	AppendCollisionGeometryRectangle(Min, Max, OutSegments);
}
