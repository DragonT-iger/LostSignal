#include "Vision/LSVisionSolver.h"

namespace
{
	// Helper for 2D ray/segment intersection math used by the polygon solver.
	float Cross2D(const FVector2D& A, const FVector2D& B)
	{
		return (A.X * B.Y) - (A.Y * B.X);
	}
}

// Builds the final 2D visibility polygon from the viewer pose and registered occluder segments.
FLSVisionPolygonData FLSVisionSolver::Solve(FLSVisionSolverInfo& SolverInfo)
{
	const float ForwardAngleDeg = FMath::RadiansToDegrees(FMath::Atan2(SolverInfo.OriginForward.Y, SolverInfo.OriginForward.X));

	TArray<float> AnglesDeg;
	AnglesDeg.Reserve(2 + (SolverInfo.Segments.Num() * 6));

	const float StartAngleDeg = ForwardAngleDeg - SolverInfo.HalfFovDegrees;
	const float EndAngleDeg = ForwardAngleDeg + SolverInfo.HalfFovDegrees;

	const auto IsAngleInsideFov = [&](const float AngleDeg)
	{
		const float DeltaDeg = FMath::FindDeltaAngleDegrees(ForwardAngleDeg, AngleDeg);
		return FMath::Abs(DeltaDeg) <= SolverInfo.HalfFovDegrees;
	};

	const auto NormalizeAngleForSort = [&](const float AngleDeg)
	{
		float RelativeDeg = FMath::FindDeltaAngleDegrees(StartAngleDeg, AngleDeg);
		if (RelativeDeg < 0.0f)
		{
			RelativeDeg += 360.0f;
		}

		return RelativeDeg;
	};

	// Always keep the FOV boundary rays so the solved polygon still matches the requested cone limits.
	AnglesDeg.Add(StartAngleDeg);
	AnglesDeg.Add(EndAngleDeg);

	TArray<FVector2D> UniqueVertices;
	UniqueVertices.Reserve(SolverInfo.Segments.Num() * 2);

	const auto AddUniqueVertex = [&UniqueVertices](const FVector2D& Vertex)
	{
		const bool bAlreadyAdded = UniqueVertices.ContainsByPredicate(
			[&Vertex](const FVector2D& ExistingVertex)
			{
				return ExistingVertex.Equals(Vertex, KINDA_SMALL_NUMBER);
			});

		if (!bAlreadyAdded)
		{
			UniqueVertices.Add(Vertex);
		}
	};

	for (const FLSVisionSegment2D* Segment : SolverInfo.Segments)
	{
		if (Segment == nullptr)
		{
			continue;
		}

		AddUniqueVertex(Segment->Start);
		AddUniqueVertex(Segment->End);
	}

	for (const FVector2D& Vertex : UniqueVertices)
	{
		const float AngleDeg = FMath::RadiansToDegrees(FMath::Atan2(Vertex.Y - SolverInfo.RayOriginPos.Y, Vertex.X - SolverInfo.RayOriginPos.X));
		if (!IsAngleInsideFov(AngleDeg))
		{
			continue;
		}

		const float LeftCandidate = AngleDeg - SolverInfo.AngleEpsilon;
		const float RightCandidate = AngleDeg + SolverInfo.AngleEpsilon;

		if (IsAngleInsideFov(LeftCandidate))
		{
			AnglesDeg.Add(LeftCandidate);
		}

		AnglesDeg.Add(AngleDeg);

		if (IsAngleInsideFov(RightCandidate))
		{
			AnglesDeg.Add(RightCandidate);
		}
	}

	AnglesDeg.Sort([&](const float A, const float B)
	{
		return NormalizeAngleForSort(A) < NormalizeAngleForSort(B);
	});

	TArray<float> UniqueAngles;
	for (const float Angle : AnglesDeg)
	{
		if (UniqueAngles.Num() == 0 ||
			!FMath::IsNearlyEqual(NormalizeAngleForSort(UniqueAngles.Last()), NormalizeAngleForSort(Angle), 0.001f))
		{
			UniqueAngles.Add(Angle);
		}
	}

	// Endpoint rays capture silhouette changes, but large empty spans still need a few samples to keep the
	// outer vision radius from collapsing into one big triangle. DivideAngleDegree now acts as the max gap.
	const float MaxAngleGapDeg = FMath::Max(SolverInfo.DivideAngleDegree, 0.1f);
	TArray<float> FinalAngles;
	FinalAngles.Reserve(UniqueAngles.Num() + FMath::CeilToInt((SolverInfo.HalfFovDegrees * 2.0f) / MaxAngleGapDeg));

	for (int32 AngleIndex = 0; AngleIndex < UniqueAngles.Num(); ++AngleIndex)
	{
		const float CurrentAngle = UniqueAngles[AngleIndex];
		FinalAngles.Add(CurrentAngle);

		if (AngleIndex == UniqueAngles.Num() - 1)
		{
			continue;
		}

		const float NextAngle = UniqueAngles[AngleIndex + 1];
		const float RelativeCurrent = NormalizeAngleForSort(CurrentAngle);
		const float RelativeNext = NormalizeAngleForSort(NextAngle);
		const float GapDeg = RelativeNext - RelativeCurrent;

		if (GapDeg <= MaxAngleGapDeg)
		{
			continue;
		}

		const int32 AdditionalSampleCount = FMath::FloorToInt(GapDeg / MaxAngleGapDeg);
		for (int32 SampleIndex = 1; SampleIndex <= AdditionalSampleCount; ++SampleIndex)
		{
			const float InsertedRelativeAngle = RelativeCurrent + (MaxAngleGapDeg * SampleIndex);
			if (InsertedRelativeAngle >= RelativeNext - KINDA_SMALL_NUMBER)
			{
				break;
			}

			FinalAngles.Add(StartAngleDeg + InsertedRelativeAngle);
		}
	}

	FinalAngles.Sort([&](const float A, const float B)
	{
		return NormalizeAngleForSort(A) < NormalizeAngleForSort(B);
	});

	FLSVisionPolygonData PolygonData;
	PolygonData.Origin = SolverInfo.OriginPos;
	PolygonData.RayOrigin = SolverInfo.RayOriginPos;
	PolygonData.VisionRadius = SolverInfo.VisionRadius;
	PolygonData.Extent = SolverInfo.MaxRayDistance;
	PolygonData.Points.Reserve(FinalAngles.Num() + 1);
	PolygonData.DebugRayHitPoints.Reserve(FinalAngles.Num());
	PolygonData.Points.Add(SolverInfo.RayOriginPos);

	for (const float AngleDeg : FinalAngles)
	{
		const float AngleRad = FMath::DegreesToRadians(AngleDeg);
		const FVector2D RayDir = FVector2D(FMath::Cos(AngleRad), FMath::Sin(AngleRad)).GetSafeNormal();

		FLSVisionRayHit ClosestHit;
		ClosestHit.Distance = SolverInfo.MaxRayDistance;

		for (const FLSVisionSegment2D* Segment : SolverInfo.Segments)
		{
			if (Segment == nullptr)
			{
				continue;
			}

			const FLSVisionRayHit Hit = CastRay(SolverInfo.RayOriginPos, RayDir, Segment->Start, Segment->End, SolverInfo.MaxRayDistance);
			if (Hit.bHit && Hit.Distance < ClosestHit.Distance)
			{
				ClosestHit = Hit;
			}
		}

		if (!ClosestHit.bHit)
		{
			ClosestHit.HitPoint = SolverInfo.RayOriginPos + (RayDir * SolverInfo.MaxRayDistance);
			ClosestHit.Distance = SolverInfo.MaxRayDistance;
			ClosestHit.bHit = true;
		}

		PolygonData.Points.Add(ClosestHit.HitPoint);
		PolygonData.DebugRayHitPoints.Add(ClosestHit.HitPoint);
	}

	return PolygonData;
}

// Tests a single ray against one segment and returns the closest valid hit on that segment.
FLSVisionRayHit FLSVisionSolver::CastRay(
	const FVector2D& Origin,
	const FVector2D& RayDir,
	const FVector2D& StartVertex,
	const FVector2D& EndVertex,
	const float MaxRayDistance)
{
	FLSVisionRayHit Result;
	Result.Distance = MaxRayDistance;

	const FVector2D SegmentDir = EndVertex - StartVertex;
	const FVector2D StartToOrigin = StartVertex - Origin;
	const float Denominator = Cross2D(RayDir, SegmentDir);

	if (FMath::IsNearlyZero(Denominator, KINDA_SMALL_NUMBER))
	{
		return Result;
	}

	const float T = Cross2D(StartToOrigin, SegmentDir) / Denominator;
	const float U = Cross2D(StartToOrigin, RayDir) / Denominator;

	const bool bHitRay = T >= 0.0f;
	const bool bWithinMaxDistance = T <= MaxRayDistance;
	const bool bHitSegment = U >= 0.0f && U <= 1.0f;

	if (!(bHitRay && bWithinMaxDistance && bHitSegment))
	{
		return Result;
	}

	Result.HitPoint = Origin + (RayDir * T);
	Result.Distance = T;
	Result.bHit = true;
	return Result;
}
