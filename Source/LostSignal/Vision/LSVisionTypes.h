#pragma once

#include "CoreMinimal.h"
#include "LSVisionTypes.generated.h"

USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSVisionSegment2D
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision")
	FVector2D Start = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision")
	FVector2D End = FVector2D::ZeroVector;
};

USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSVisionRayHit
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision")
	FVector2D HitPoint = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision")
	float Distance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision")
	bool bHit = false;
};

USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSVisionPolygonData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision")
	float VisionRadius = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision")
	float Extent = 2500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision")
	FVector2D Origin = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision")
	TArray<FVector2D> Points;

	// Stores the exact ray hit points used while solving visibility so debug drawing can show
	// actual raycasts without reconstructing them from the polygon outline.
	UPROPERTY(Transient)
	TArray<FVector2D> DebugRayHitPoints;
};

USTRUCT()
struct LOSTSIGNAL_API FLSVisionSolverInfo
{
	GENERATED_BODY()

	FVector2D OriginPos = FVector2D::ZeroVector;
	FVector2D OriginForward = FVector2D::UnitX();
	TArray<FLSVisionSegment2D*> Segments;
	float HalfFovDegrees = 45.0f;
	float DivideAngleDegree = 2.0f;
	float VisionRadius = 100.0f;
	float AngleEpsilon = 0.01f;
	float MaxRayDistance = 2500.0f;
	UWorld* World = nullptr;
};
