#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Vision/LSVisionTypes.h"
#include "LSVisionSubsystem.generated.h"

class ALSVisionMaskRenderer;
class ULSVisionOccluderComponent;
class ULSVisionSurfaceComponent;
class ULSVisionTargetComponent;
class UTextureRenderTarget2D;

UCLASS()
class LOSTSIGNAL_API ULSVisionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void RegisterOccluder(ULSVisionOccluderComponent* Occluder);
	void UnregisterOccluder(ULSVisionOccluderComponent* Occluder);
	void RefreshOccluder(ULSVisionOccluderComponent* Occluder);

	void RegisterSurface(ULSVisionSurfaceComponent* Surface);
	void UnregisterSurface(ULSVisionSurfaceComponent* Surface);

	void RegisterTarget(ULSVisionTargetComponent* Target);
	void UnregisterTarget(ULSVisionTargetComponent* Target);
	void QuerySegmentsInRadius(const FVector2D& Origin, float Radius, TArray<FLSVisionSegment2D*>& OutSegments) const;

	const TArray<TObjectPtr<ULSVisionOccluderComponent>>& GetRegisteredOccluders() const
	{
		return RegisteredOccluders;
	}

	const TArray<TObjectPtr<ULSVisionSurfaceComponent>>& GetRegisteredVisionSurfaces() const
	{
		return RegisteredSurfaces;
	}

	const TArray<TObjectPtr<ULSVisionTargetComponent>>& GetRegisteredVisionTargets() const
	{
		return RegisteredTargets;
	}

	ALSVisionMaskRenderer* GetMaskRenderer() const
	{
		return MaskRenderer;
	}

	UTextureRenderTarget2D* GetVisibilityMaskRenderTarget() const
	{
		return RuntimeMaskRenderTarget;
	}

	// 오클루더 세그먼트(시야 차단 형상)가 추가/제거/이동될 때마다 증가한다.
	// 플레이어가 가만히 있어도 이 값이 바뀌면 시야를 다시 계산해야 한다.
	int32 GetSegmentTopologyVersion() const
	{
		return SegmentTopologyVersion;
	}

	// 오클루더 콜리전을 자르는 시야 평면 월드 Z. 플레이어 발 높이 기준 모드에서 런타임에 갱신된다.
	float GetRuntimeSliceZ() const
	{
		return RuntimeSliceZ;
	}

	// 시야 평면 Z를 갱신하고, 의미 있게 바뀌었으면 등록된 모든 오클루더의 단면을 다시 계산한다.
	void SetRuntimeSliceZ(float NewSliceZ);

private:
	UTextureRenderTarget2D* ResolveVisibilityMaskRenderTarget();
	UTextureRenderTarget2D* CreateRenderTargetFromTemplate(const UTextureRenderTarget2D* TemplateRenderTarget);
	UTextureRenderTarget2D* CreateFallbackRenderTarget(int32 Size);
	FLSVisionGridCellKey WorldToGridCell(const FVector2D& Position) const;
	void CollectGridCellsForBounds(const FBox2D& Bounds, TArray<FLSVisionGridCellKey>& OutCells) const;
	void RegisterOccluderInGrid(ULSVisionOccluderComponent* Occluder);
	void UnregisterOccluderFromGrid(ULSVisionOccluderComponent* Occluder);

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULSVisionOccluderComponent>> RegisteredOccluders;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULSVisionSurfaceComponent>> RegisteredSurfaces;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULSVisionTargetComponent>> RegisteredTargets;

	UPROPERTY(Transient)
	TObjectPtr<ALSVisionMaskRenderer> MaskRenderer;

	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> RuntimeMaskRenderTarget;

	TMap<FLSVisionGridCellKey, FLSVisionGridCell> GridCells;

	TMap<int32, FLSVisionCachedSegment> CachedSegments;

	TMap<TWeakObjectPtr<ULSVisionOccluderComponent>, FLSVisionOccluderGridState> OccluderGridStates;

	int32 NextSegmentId = 0;

	int32 SegmentTopologyVersion = 0;

	float RuntimeSliceZ = 0.0f;

	float GridCellSize = 800.0f;
};
