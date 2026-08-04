#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Vision/LSVisionTypes.h"
#include "LSVisionSubsystem.generated.h"

class ALSVisionMaskRenderer;
class UMaterialParameterCollection;
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
	// 세그먼트를 값으로 복사해 넘긴다. 내부 캐시(TMap) 포인터를 반환하면 캐시 재배치 시 무효화되기 때문이다.
	void QuerySegmentsInRadius(const FVector2D& Origin, float Radius, TArray<FLSVisionSegment2D>& OutSegments) const;

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

	// 등록된 서피스 집합이 바뀔 때마다 증가한다(WP 스트리밍 인/아웃 등).
	// 폴리곤은 그대로여도 새 서피스는 마스크 파라미터를 아직 못 받았으므로, 이 값이 바뀌면 파라미터를 다시 푸시해야 한다.
	int32 GetSurfaceRegistryVersion() const
	{
		return SurfaceRegistryVersion;
	}

	// 오클루더 콜리전을 자르는 시야 평면 월드 Z. 플레이어 발 높이 기준 모드에서 런타임에 갱신된다.
	float GetRuntimeSliceZ() const
	{
		return RuntimeSliceZ;
	}

	// 시야 평면 Z를 갱신하고, 의미 있게 바뀌었으면 등록된 모든 오클루더의 단면을 다시 계산한다.
	void SetRuntimeSliceZ(float NewSliceZ);

	// 마스크 배치값을 머티리얼 파라미터 컬렉션에 쓴다. 컬렉션 인스턴스는 world마다 따로이므로 값은 자동으로 격리된다.
	// 서피스별 MID 푸시와 달리 프레임당 업로드가 서피스 수와 무관하게 1회다.
	void ApplyVisionParametersToCollection(const FVector& MaskOriginWS, float MaskExtent, float SurfacePush);

private:
	UTextureRenderTarget2D* ResolveVisibilityMaskRenderTarget();
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

	// 설정 쪽은 TSoftObjectPtr이라 프레임당 읽으면 LoadSynchronous가 되므로 Initialize에서 한 번 해석해 담아둔다.
	// (EdgeNoiseTexture와 같은 패턴.) GC가 추적할 수 있도록 UPROPERTY로 유지한다.
	UPROPERTY(Transient)
	TObjectPtr<UMaterialParameterCollection> VisionParameterCollection;

	// 컬렉션에 없는 파라미터 이름을 매 프레임 경고하지 않도록 1회만 찍기 위한 상태.
	bool bWarnedMissingCollectionParameter = false;

	// 마스크 RT(에셋)를 공유하는 동시 활성 비전 world를 이 프로세스에서 셌는지 여부. 증감 짝을 보장하기 위한 플래그.
	bool bCountedActiveVisionWorld = false;

	TMap<FLSVisionGridCellKey, FLSVisionGridCell> GridCells;

	TMap<int32, FLSVisionCachedSegment> CachedSegments;

	TMap<TWeakObjectPtr<ULSVisionOccluderComponent>, FLSVisionOccluderGridState> OccluderGridStates;

	int32 NextSegmentId = 0;

	int32 SegmentTopologyVersion = 0;

	int32 SurfaceRegistryVersion = 0;

	float RuntimeSliceZ = 0.0f;

	float GridCellSize = 800.0f;
};
