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
	AnglesDeg.Reserve(2 + FMath::CeilToInt((SolverInfo.HalfFovDegrees * 2.0f) / SolverInfo.DivideAngleDegree) + (SolverInfo.Segments.Num() * 3));

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

	for (float Angle = StartAngleDeg; Angle <= EndAngleDeg; Angle += SolverInfo.DivideAngleDegree)
	{
		AnglesDeg.Add(Angle);
	}

	AnglesDeg.Add(EndAngleDeg);

	TArray<FIntPoint> UniqueVertices;
	UniqueVertices.Reserve(SolverInfo.Segments.Num() * 2);

	for (const FLSVisionSegment2D* Segment : SolverInfo.Segments)
	{
		if (Segment == nullptr)
		{
			continue;
		}

		UniqueVertices.AddUnique(FIntPoint(FMath::RoundToInt(Segment->Start.X), FMath::RoundToInt(Segment->Start.Y)));
		UniqueVertices.AddUnique(FIntPoint(FMath::RoundToInt(Segment->End.X), FMath::RoundToInt(Segment->End.Y)));
	}

	for (const FIntPoint& Vertex : UniqueVertices)
	{
		const float AngleDeg = FMath::RadiansToDegrees(FMath::Atan2(Vertex.Y - SolverInfo.OriginPos.Y, Vertex.X - SolverInfo.OriginPos.X));
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

	FLSVisionPolygonData PolygonData;
	PolygonData.Origin = SolverInfo.OriginPos;
	PolygonData.VisionRadius = SolverInfo.VisionRadius;
	PolygonData.Extent = SolverInfo.MaxRayDistance;
	PolygonData.Points.Reserve(UniqueAngles.Num() + 1);
	PolygonData.Points.Add(SolverInfo.OriginPos);

	for (const float AngleDeg : UniqueAngles)
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

			const FLSVisionRayHit Hit = CastRay(SolverInfo.OriginPos, RayDir, Segment->Start, Segment->End, SolverInfo.MaxRayDistance);
			if (Hit.bHit && Hit.Distance < ClosestHit.Distance)
			{
				ClosestHit = Hit;
			}
		}

		if (!ClosestHit.bHit)
		{
			ClosestHit.HitPoint = SolverInfo.OriginPos + (RayDir * SolverInfo.MaxRayDistance);
			ClosestHit.Distance = SolverInfo.MaxRayDistance;
			ClosestHit.bHit = true;
		}

		PolygonData.Points.Add(ClosestHit.HitPoint);
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
