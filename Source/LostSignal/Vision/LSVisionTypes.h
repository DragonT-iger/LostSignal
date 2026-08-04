#pragma once

#include "CoreMinimal.h"
#include "LSVisionTypes.generated.h"

class ULSVisionOccluderComponent;

namespace LSVisionTags
{
	// 이 ComponentTag가 달린 프리미티브는 시야 밖 숨김(VisionTarget) 대상에서 제외한다.
	// 예: 몬스터 텔레그래프 프리뷰 — 시전자가 시야 밖이어도 위험 범위는 항상 표시.
	inline const FName HideExempt(TEXT("LSVisionHideExempt"));
}

USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSVisionSegment2D
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision")
	FVector2D Start = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision")
	FVector2D End = FVector2D::ZeroVector;
};

USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSVisionRayHit
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision")
	FVector2D HitPoint = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision")
	float Distance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision")
	bool bHit = false;
};

USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSVisionPolygonData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision")
	float VisionRadius = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision")
	float Extent = 2500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision")
	FVector2D Origin = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision")
	FVector2D RayOrigin = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision")
	TArray<FVector2D> Points;

	// Points와 1:1로 대응. 해당 점이 오클루더(벽)에 맞은 점이면 1.0, 아무것도 못 맞고 최대거리/반경에
	// 도달한 "열린" 점(또는 apex)이면 0.0. 셰이더가 바깥 시야 경계 엣지에만 페더를 적용할 때 사용한다.
	UPROPERTY(Transient)
	TArray<float> PointFlags;
};

USTRUCT()
struct LOSTSIGNAL_API FLSVisionSolverInfo
{
	GENERATED_BODY()

	FVector2D OriginPos = FVector2D::ZeroVector;
	FVector2D RayOriginPos = FVector2D::ZeroVector;
	FVector2D OriginForward = FVector2D::UnitX();
	// 값 소유. 서브시스템의 세그먼트 캐시(TMap)를 가리키는 포인터를 들고 있으면 캐시가 재배치될 때
	// 무효화되므로, 쿼리 시점에 복사해 이 구조체가 직접 소유한다.
	TArray<FLSVisionSegment2D> Segments;
	float HalfFovDegrees = 45.0f;
	float DivideAngleDegree = 2.0f;
	float VisionRadius = 100.0f;
	float AngleEpsilon = 0.01f;
	float MaxRayDistance = 2500.0f;
};

USTRUCT()
struct LOSTSIGNAL_API FLSVisionGridCellKey
{
	GENERATED_BODY()

	int32 X = 0;
	int32 Y = 0;

	FLSVisionGridCellKey() = default;

	FLSVisionGridCellKey(const int32 InX, const int32 InY)
		: X(InX)
		, Y(InY)
	{
	}

	bool operator==(const FLSVisionGridCellKey& Other) const
	{
		return X == Other.X && Y == Other.Y;
	}
};

FORCEINLINE uint32 GetTypeHash(const FLSVisionGridCellKey& Key)
{
	return HashCombine(GetTypeHash(Key.X), GetTypeHash(Key.Y));
}

USTRUCT()
struct LOSTSIGNAL_API FLSVisionGridCell
{
	GENERATED_BODY()

	TArray<int32> SegmentIds;
};

USTRUCT()
struct LOSTSIGNAL_API FLSVisionCachedSegment
{
	GENERATED_BODY()

	int32 SegmentId = INDEX_NONE;
	TWeakObjectPtr<ULSVisionOccluderComponent> Owner;
	FLSVisionSegment2D Segment;
	FBox2D Bounds = FBox2D(EForceInit::ForceInitToZero);
};

USTRUCT()
struct LOSTSIGNAL_API FLSVisionOccluderGridState
{
	GENERATED_BODY()

	TArray<int32> SegmentIds;
	TArray<FLSVisionGridCellKey> OccupiedCells;
};
