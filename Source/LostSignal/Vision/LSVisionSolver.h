#pragma once

#include "CoreMinimal.h"
#include "Vision/LSVisionTypes.h"

class LOSTSIGNAL_API FLSVisionSolver
{
public:
	static FLSVisionPolygonData Solve(FLSVisionSolverInfo& SolverInfo);
	static FLSVisionRayHit CastRay(
		const FVector2D& Origin,
		const FVector2D& RayDir,
		const FVector2D& StartVertex,
		const FVector2D& EndVertex,
		float MaxRayDistance);
};
