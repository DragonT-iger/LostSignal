#include "Vision/LSVisionSolver.h"

namespace
{
	// Helper for 2D ray/segment intersection math used by the polygon solver.
	float CrossVisionSolver2D(const FVector2D& A, const FVector2D& B)
	{
		return (A.X * B.Y) - (A.Y * B.X);
	}

	bool IsPointBehindVisionSolverViewer(const FVector2D& ViewerOrigin, const FVector2D& ViewerForward, const FVector2D& Point)
	{
		return FVector2D::DotProduct(Point - ViewerOrigin, ViewerForward) < -KINDA_SMALL_NUMBER;
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

	// A. FOV 콘 밖 세그먼트 선컬링.
	// 그리드는 반경 전체(MaxRayDistance) 원판을 긁어오지만 폴리곤은 HalfFov 콘만 쓴다.
	// 콘과 각도 구간이 겹치지 않는 세그먼트는 어떤 콘 레이도 막을 수 없으므로 미리 제거해 레이 루프 N을 줄인다.
	// 양 끝점의 forward 기준 부호각 [min,max]가 콘 반각을 벗어나면 컬링. 뒤쪽 래핑(±180)은 안전하게 살려두고
	// 아래 히트 폐기 규칙(가르는 벽의 평면 뒤 히트만 무시)이 걸러내므로 결과는 불변, 효율만 얻는다.
	// 컬링을 통과한 세그먼트에는 "그 직선이 캐릭터(OriginPos)와 apex(RayOriginPos)를 서로 반대편으로
	// 가르는지"를 함께 캐시한다. 가르는 벽 = apex와 캐릭터 사이의 벽으로, 전방 시야를 막으면 안 된다.
	struct FLSCulledVisionSegment
	{
		const FLSVisionSegment2D* Segment = nullptr;
		bool bSeparatesViewerFromRayOrigin = false;
		// apex 기준 각도 구간(forward 기준 부호각). 이 구간 밖으로 나가는 레이는 이 세그먼트를 맞출 수 없다.
		float MinDeltaDeg = 0.0f;
		float MaxDeltaDeg = 0.0f;
		// 구간 폭이 180도를 넘으면 ±180 래핑이라 [Min,Max]가 실제 각도 범위를 뒤집어 표현한다.
		// 이 경우 구간 컬링을 신뢰할 수 없으므로 항상 교차 검사한다(컬링하면 벽이 뚫린다).
		bool bAlwaysTest = false;
	};
	TArray<FLSCulledVisionSegment> CulledSegments;
	CulledSegments.Reserve(SolverInfo.Segments.Num());
	{
		const float CullHalfFovDeg = SolverInfo.HalfFovDegrees + SolverInfo.AngleEpsilon;
		for (const FLSVisionSegment2D* Segment : SolverInfo.Segments)
		{
			if (Segment == nullptr)
			{
				continue;
			}

			const float StartDeltaDeg = FMath::FindDeltaAngleDegrees(
				ForwardAngleDeg,
				FMath::RadiansToDegrees(FMath::Atan2(Segment->Start.Y - SolverInfo.RayOriginPos.Y, Segment->Start.X - SolverInfo.RayOriginPos.X)));
			const float EndDeltaDeg = FMath::FindDeltaAngleDegrees(
				ForwardAngleDeg,
				FMath::RadiansToDegrees(FMath::Atan2(Segment->End.Y - SolverInfo.RayOriginPos.Y, Segment->End.X - SolverInfo.RayOriginPos.X)));

			const float MinDeltaDeg = FMath::Min(StartDeltaDeg, EndDeltaDeg);
			const float MaxDeltaDeg = FMath::Max(StartDeltaDeg, EndDeltaDeg);

			if (MaxDeltaDeg < -CullHalfFovDeg || MinDeltaDeg > CullHalfFovDeg)
			{
				continue;
			}

			// 캐릭터/apex가 세그먼트 직선의 서로 반대편이면 가름. 어느 한쪽이 직선 위(≈0)면
			// 가르지 않는 것으로 취급 — 차폐를 유지하는 쪽이 안전하다.
			const FVector2D SegmentDir = Segment->End - Segment->Start;
			const float OriginSide = CrossVisionSolver2D(SegmentDir, SolverInfo.OriginPos - Segment->Start);
			const float RayOriginSide = CrossVisionSolver2D(SegmentDir, SolverInfo.RayOriginPos - Segment->Start);
			const bool bSeparatesViewer = (OriginSide * RayOriginSide) < -KINDA_SMALL_NUMBER;

			// 여기서 구한 각도 구간을 버리지 않고 들고 간다. 아래 레이 루프에서 레이별 사전 컬링에 재사용한다.
			CulledSegments.Add({
				Segment,
				bSeparatesViewer,
				MinDeltaDeg,
				MaxDeltaDeg,
				(MaxDeltaDeg - MinDeltaDeg) > 180.0f});
		}
	}

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
	UniqueVertices.Reserve(CulledSegments.Num() * 2);

	// B. dedup을 양자화 키 TSet으로. 정점마다 배열 전체를 선형 탐색하던 O(V²)를 O(V)로 낮춘다.
	// KINDA_SMALL_NUMBER(1e-4) 격자에 스냅한 정수 키로 중복 판정 — 동일 위치 정점이 한 번만 들어간다.
	TSet<FIntPoint> SeenVertexKeys;
	SeenVertexKeys.Reserve(CulledSegments.Num() * 2);

	const auto AddUniqueVertex = [&UniqueVertices, &SeenVertexKeys, &SolverInfo](const FVector2D& Vertex, const bool bSegmentSeparatesViewer)
	{
		// 가르는 벽(apex-캐릭터 사이)의 평면 뒤 꼭짓점은 히트도 폐기되므로 실루엣 후보에서 제외한다.
		// 같은 편 벽은 평면 뒤 꼭짓점도 유효한 실루엣이라 살린다(옆 벽 밀착 시 경계 정확도).
		if (bSegmentSeparatesViewer && IsPointBehindVisionSolverViewer(SolverInfo.OriginPos, SolverInfo.OriginForward, Vertex))
		{
			return;
		}

		const FIntPoint VertexKey(
			FMath::RoundToInt(Vertex.X / KINDA_SMALL_NUMBER),
			FMath::RoundToInt(Vertex.Y / KINDA_SMALL_NUMBER));

		bool bAlreadyAdded = false;
		SeenVertexKeys.Add(VertexKey, &bAlreadyAdded);
		if (!bAlreadyAdded)
		{
			UniqueVertices.Add(Vertex);
		}
	};

	for (const FLSCulledVisionSegment& Culled : CulledSegments)
	{
		AddUniqueVertex(Culled.Segment->Start, Culled.bSeparatesViewerFromRayOrigin);
		AddUniqueVertex(Culled.Segment->End, Culled.bSeparatesViewerFromRayOrigin);
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
	PolygonData.PointFlags.Reserve(FinalAngles.Num() + 1);
	PolygonData.DebugRayHitPoints.Reserve(FinalAngles.Num());
	PolygonData.Points.Add(SolverInfo.RayOriginPos);
	PolygonData.PointFlags.Add(0.0f); // apex(RayOrigin)는 열린 점으로 취급.

	for (const float AngleDeg : FinalAngles)
	{
		const float AngleRad = FMath::DegreesToRadians(AngleDeg);
		const FVector2D RayDir = FVector2D(FMath::Cos(AngleRad), FMath::Sin(AngleRad)).GetSafeNormal();

		// 레이별 각도 컬링용. 세그먼트 구간과 같은 기준(forward 기준 부호각, apex 원점)이라 그대로 비교할 수 있다.
		const float RayDeltaDeg = FMath::FindDeltaAngleDegrees(ForwardAngleDeg, AngleDeg);

		FLSVisionRayHit ClosestHit;
		ClosestHit.Distance = SolverInfo.MaxRayDistance;

		for (const FLSCulledVisionSegment& Culled : CulledSegments)
		{
			// 레이 각도가 세그먼트의 각도 구간 밖이면 교차할 수 없다. 실루엣 정점 양옆에 삽입된 ±AngleEpsilon
			// 레이가 경계에서 잘리지 않도록 같은 폭만큼 여유를 준다. 결과는 불변이고 교차 판정 횟수만 줄어든다.
			if (!Culled.bAlwaysTest
				&& (RayDeltaDeg < Culled.MinDeltaDeg - SolverInfo.AngleEpsilon
					|| RayDeltaDeg > Culled.MaxDeltaDeg + SolverInfo.AngleEpsilon))
			{
				continue;
			}

			const FLSVisionRayHit Hit = CastRay(SolverInfo.RayOriginPos, RayDir, Culled.Segment->Start, Culled.Segment->End, SolverInfo.MaxRayDistance);
			if (!Hit.bHit)
			{
				continue;
			}

			// apex가 캐릭터 뒤에 있어 경계 레이는 벽을 캐릭터 평면 뒤에서 히트할 수 있다.
			// 평면 뒤 히트라도 같은 편 벽(옆 벽 밀착 등)은 실제로 시선을 막으므로 차폐로 인정하고,
			// apex와 캐릭터를 가르는 벽(등 뒤 벽)의 히트만 무시한다 — 벽 밀착 시 경계 레이 뚫림 수정.
			if (Culled.bSeparatesViewerFromRayOrigin && IsPointBehindVisionSolverViewer(SolverInfo.OriginPos, SolverInfo.OriginForward, Hit.HitPoint))
			{
				continue;
			}

			if (Hit.Distance < ClosestHit.Distance)
			{
				ClosestHit = Hit;
			}
		}

		// max-distance 합성으로 bHit를 덮어쓰기 전에 "실제 오클루더에 맞았는지"를 캡처한다.
		const bool bWasOccluderHit = ClosestHit.bHit;

		if (!ClosestHit.bHit)
		{
			ClosestHit.HitPoint = SolverInfo.RayOriginPos + (RayDir * SolverInfo.MaxRayDistance);
			ClosestHit.Distance = SolverInfo.MaxRayDistance;
			ClosestHit.bHit = true;
		}

		PolygonData.Points.Add(ClosestHit.HitPoint);
		PolygonData.PointFlags.Add(bWasOccluderHit ? 1.0f : 0.0f);
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
	const float Denominator = CrossVisionSolver2D(RayDir, SegmentDir);

	if (FMath::IsNearlyZero(Denominator, KINDA_SMALL_NUMBER))
	{
		return Result;
	}

	const float T = CrossVisionSolver2D(StartToOrigin, SegmentDir) / Denominator;
	const float U = CrossVisionSolver2D(StartToOrigin, RayDir) / Denominator;

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
