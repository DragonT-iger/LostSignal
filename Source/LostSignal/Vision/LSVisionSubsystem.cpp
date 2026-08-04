#include "Vision/LSVisionSubsystem.h"

#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "LostSignal.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "RenderingThread.h"
#include "Vision/LSVisionMaskRenderer.h"
#include "Vision/LSVisionOccluderComponent.h"
#include "Vision/LSVisionSettings.h"
#include "Vision/LSVisionTargetComponent.h"

namespace
{
	float ComputeSquaredDistanceToBounds(const FVector2D& Point, const FBox2D& Bounds)
	{
		const float DeltaX = Point.X < Bounds.Min.X ? Bounds.Min.X - Point.X : (Point.X > Bounds.Max.X ? Point.X - Bounds.Max.X : 0.0f);
		const float DeltaY = Point.Y < Bounds.Min.Y ? Bounds.Min.Y - Point.Y : (Point.Y > Bounds.Max.Y ? Point.Y - Bounds.Max.Y : 0.0f);
		return FMath::Square(DeltaX) + FMath::Square(DeltaY);
	}

	// [마스크 RT 계약] 아래 세 설정은 취향이 아니라 정확성 요구사항이므로 에셋 저작값에 맡기지 않고 코드가 강제한다.
	// 이 계약의 단일 출처는 이 함수다. 템플릿 경로와 폴백 경로가 모두 여기를 거친다.
	//
	// - bCanCreateUAV: 컴퓨트 셰이더가 RWTexture2D로 직접 쓰므로 UAV 생성이 가능해야 한다.
	// - TA_Clamp: 서피스 머티리얼은 등록된 모든 벽·바닥의 모든 픽셀에서 MaskOriginWS/MaskExtent로 UV를 계산하므로,
	//   마스크 창(±Extent) 밖 지오메트리는 필연적으로 UV가 [0,1]을 벗어난다. Wrap이면 창 반대편을 읽어 멀리 떨어진
	//   지오메트리에 시야 콘이 격자처럼 복제된다. Clamp면 테두리 텍셀(=가려짐)을 읽어 균일하게 가려진 상태로 퇴화한다.
	// - bAutoGenerateMips 금지: 컴퓨트가 mip 0만 쓰므로 상위 mip이 있으면 머티리얼이 갱신되지 않은 mip을 샘플할 수 있다.
	void ApplyVisionMaskRenderTargetContract(UTextureRenderTarget2D& RenderTarget)
	{
		RenderTarget.bCanCreateUAV = true;
		RenderTarget.AddressX = TextureAddress::TA_Clamp;
		RenderTarget.AddressY = TextureAddress::TA_Clamp;
		RenderTarget.bAutoGenerateMips = false;
	}

	// 설정 에셋을 복사 없이 직접 쓰는 경로용. 계약 위반은 에셋을 고쳐야 하는 문제이므로 조치를 Error로 남기고,
	// 부팅을 막지 않기 위해 런타임에 한 번 강제한다. bCanCreateUAV는 리소스 생성 전 상태여야 하므로 강제 후 재생성한다.
	// Modify()를 부르지 않으므로 에디터에서 에셋이 dirty로 표시되지는 않는다.
	void EnforceVisionMaskContractOnConfiguredAsset(UTextureRenderTarget2D& RenderTarget)
	{
		const bool bViolatesContract =
			!RenderTarget.bCanCreateUAV
			|| RenderTarget.AddressX != TextureAddress::TA_Clamp
			|| RenderTarget.AddressY != TextureAddress::TA_Clamp
			|| RenderTarget.bAutoGenerateMips;

		if (!bViolatesContract)
		{
			return;
		}

		UE_LOG(LogLS, Error,
			TEXT("Vision mask RT 에셋 '%s'이 계약을 위반합니다(런타임 강제 적용). 에셋을 이렇게 고쳐 주세요 — bCanCreateUAV=true(현재 %s), AddressX/Y=Clamp(현재 %d/%d), bAutoGenerateMips=false(현재 %s)."),
			*GetNameSafe(&RenderTarget),
			RenderTarget.bCanCreateUAV ? TEXT("true") : TEXT("false"),
			static_cast<int32>(RenderTarget.AddressX),
			static_cast<int32>(RenderTarget.AddressY),
			RenderTarget.bAutoGenerateMips ? TEXT("true") : TEXT("false"));

		ApplyVisionMaskRenderTargetContract(RenderTarget);
		RenderTarget.UpdateResourceImmediate(true);
	}

	// 마스크 RT는 설정 에셋을 직접 쓰므로 프로세스 전역이다. 한 프로세스에서 두 world가 동시에 시야를 굴리면
	// (에디터 PIE 다중 클라이언트) 서로의 마스크를 덮어써 깜빡인다. 조용히 깨지지 않도록 세어서 경고한다.
	int32 GLSActiveVisionWorldCount = 0;
}

// Creates the shared runtime objects that every local vision calculation depends on.
void ULSVisionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const ULSVisionSettings* VisionSettings = GetDefault<ULSVisionSettings>();
	UClass* MaskRendererClass = ALSVisionMaskRenderer::StaticClass();

	if (VisionSettings != nullptr && !VisionSettings->MaskRendererClass.IsNull())
	{
		if (UClass* LoadedClass = VisionSettings->MaskRendererClass.LoadSynchronous())
		{
			MaskRendererClass = LoadedClass;
		}
	}

	RuntimeMaskRenderTarget = ResolveVisibilityMaskRenderTarget();

	if (UWorld* World = GetWorld(); World != nullptr && MaskRendererClass != nullptr)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.ObjectFlags |= RF_Transient;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		MaskRenderer = World->SpawnActor<ALSVisionMaskRenderer>(MaskRendererClass, FTransform::Identity, SpawnParameters);
		if (MaskRenderer != nullptr)
		{
			MaskRenderer->SetActorHiddenInGame(true);
			MaskRenderer->SetCanBeDamaged(false);
			MaskRenderer->VisibilityMaskRenderTarget = RuntimeMaskRenderTarget;

			// 스칼라·색 값은 렌더러가 디스패치 시점에 설정에서 직접 읽으므로 복사하지 않는다.
			// 노이즈 텍스쳐만 여기서 한 번 동기 로드해 넘긴다(프레임당 LoadSynchronous 방지).
			if (VisionSettings != nullptr)
			{
				MaskRenderer->EdgeNoiseTexture = VisionSettings->EdgeNoiseTexture.LoadSynchronous();
			}

			// 이 world가 마스크 RT(전역 에셋)에 쓰기 시작했다. 두 개 이상이면 서로 덮어쓰므로 원인을 남긴다.
			bCountedActiveVisionWorld = true;
			++GLSActiveVisionWorldCount;
			if (GLSActiveVisionWorldCount > 1)
			{
				UE_LOG(LogLS, Warning,
					TEXT("이 프로세스에서 시야를 굴리는 world가 %d개입니다. 마스크 RT는 설정 에셋을 직접 쓰므로 공유되어 서로 덮어씁니다 — PIE 다중 클라이언트는 별도 프로세스로 실행하세요."),
					GLSActiveVisionWorldCount);
			}
		}
	}

	// 컬렉션도 프레임당 LoadSynchronous를 피하려고 여기서 한 번만 해석한다(EdgeNoiseTexture와 같은 이유).
	VisionParameterCollection = VisionSettings != nullptr ? VisionSettings->VisionParameterCollection.LoadSynchronous() : nullptr;
	bWarnedMissingCollectionParameter = false;

	RegisteredOccluders.Reset();
	RegisteredSurfaces.Reset();
	RegisteredTargets.Reset();
	GridCells.Reset();
	CachedSegments.Reset();
	OccluderGridStates.Reset();
	NextSegmentId = 0;
	GridCellSize = VisionSettings != nullptr
		? FMath::Max(VisionSettings->SpatialGridCellSize, 100.0f)
		: 800.0f;
	RuntimeSliceZ = VisionSettings != nullptr ? VisionSettings->OccluderSliceHeight : 0.0f;
}

// 시야 평면 Z가 의미 있게 바뀌면 등록된 모든 오클루더의 단면을 다시 계산한다.
// 평면 게임에선 시작 시 한 번만 호출되는 수준이라 비용은 무시 가능.
void ULSVisionSubsystem::SetRuntimeSliceZ(const float NewSliceZ)
{
	if (FMath::IsNearlyEqual(RuntimeSliceZ, NewSliceZ, 1.0f))
	{
		return;
	}

	RuntimeSliceZ = NewSliceZ;

	for (ULSVisionOccluderComponent* Occluder : RegisteredOccluders)
	{
		if (Occluder != nullptr)
		{
			Occluder->RebuildSegments();
		}
	}
}

// 마스크 배치값을 머티리얼 파라미터 컬렉션에 쓴다. 이름은 MPC 에셋과 반드시 일치해야 하는 계약이다.
void ULSVisionSubsystem::ApplyVisionParametersToCollection(const FVector& MaskOriginWS, const float MaskExtent, const float SurfacePush)
{
	if (VisionParameterCollection == nullptr)
	{
		return;
	}

	UWorld* World = GetWorld();
	UMaterialParameterCollectionInstance* CollectionInstance = World != nullptr
		? World->GetParameterCollectionInstance(VisionParameterCollection)
		: nullptr;
	if (CollectionInstance == nullptr)
	{
		return;
	}

	// Set*ParameterValue는 컬렉션에 그 이름이 없으면 false를 반환한다. MPC 에셋 세팅 실수를 조용히 넘기지 않는다.
	bool bAllParametersFound = CollectionInstance->SetVectorParameterValue(
		TEXT("MaskOriginWS"),
		FLinearColor(MaskOriginWS.X, MaskOriginWS.Y, MaskOriginWS.Z, 0.0f));
	bAllParametersFound &= CollectionInstance->SetScalarParameterValue(TEXT("MaskExtent"), MaskExtent);
	bAllParametersFound &= CollectionInstance->SetScalarParameterValue(TEXT("MaskSurfacePush"), SurfacePush);

	if (!bAllParametersFound && !bWarnedMissingCollectionParameter)
	{
		bWarnedMissingCollectionParameter = true;
		UE_LOG(LogLS, Warning,
			TEXT("Vision 파라미터 컬렉션 '%s'에 필요한 파라미터가 없습니다. Vector MaskOriginWS / Scalar MaskExtent / Scalar MaskSurfacePush를 정확한 이름으로 추가해 주세요."),
			*GetNameSafe(VisionParameterCollection));
	}
}

// Releases the shared vision runtime objects when the world is torn down.
void ULSVisionSubsystem::Deinitialize()
{
	RegisteredOccluders.Reset();
	RegisteredSurfaces.Reset();
	RegisteredTargets.Reset();
	GridCells.Reset();
	CachedSegments.Reset();
	OccluderGridStates.Reset();
	NextSegmentId = 0;

	// Drain any queued mask updates before tearing down the renderer and its RT resource.
	FlushRenderingCommands();

	if (IsValid(MaskRenderer))
	{
		MaskRenderer->VisibilityMaskRenderTarget = nullptr;
		MaskRenderer->Destroy();
		MaskRenderer = nullptr;
	}

	// 렌더러가 이미 무효화됐어도 증감 짝이 어긋나지 않도록 플래그로 감소를 판단한다.
	if (bCountedActiveVisionWorld)
	{
		bCountedActiveVisionWorld = false;
		--GLSActiveVisionWorldCount;
	}

	RuntimeMaskRenderTarget = nullptr;
	VisionParameterCollection = nullptr;

	Super::Deinitialize();
}

// Tracks occluders so the solver can collect all blocking segments in one place.
void ULSVisionSubsystem::RegisterOccluder(ULSVisionOccluderComponent* Occluder)
{
	if (Occluder == nullptr)
	{
		return;
	}

	if (RegisteredOccluders.Contains(Occluder))
	{
		RefreshOccluder(Occluder);
		return;
	}

	RegisteredOccluders.AddUnique(Occluder);
	RegisterOccluderInGrid(Occluder);
}

// Removes occluders that are no longer valid for this world.
void ULSVisionSubsystem::UnregisterOccluder(ULSVisionOccluderComponent* Occluder)
{
	UnregisterOccluderFromGrid(Occluder);
	RegisteredOccluders.Remove(Occluder);
}

// Refreshes the grid membership after an occluder moved or rebuilt its source segments.
void ULSVisionSubsystem::RefreshOccluder(ULSVisionOccluderComponent* Occluder)
{
	if (Occluder == nullptr)
	{
		return;
	}

	if (!RegisteredOccluders.Contains(Occluder))
	{
		RegisterOccluder(Occluder);
		return;
	}

	UnregisterOccluderFromGrid(Occluder);
	RegisterOccluderInGrid(Occluder);
}

// Tracks surfaces that need the latest mask texture and transform parameters.
// 집합이 실제로 변했을 때만 버전을 올린다 — 새 서피스는 마스크 파라미터를 아직 못 받은 상태이므로
// 플레이어가 멈춰 있어도(폴리곤 재계산 없이) 파라미터를 다시 푸시해야 한다.
void ULSVisionSubsystem::RegisterSurface(ULSVisionSurfaceComponent* Surface)
{
	if (Surface == nullptr)
	{
		return;
	}

	const int32 PreviousSurfaceCount = RegisteredSurfaces.Num();
	RegisteredSurfaces.AddUnique(Surface);

	if (RegisteredSurfaces.Num() != PreviousSurfaceCount)
	{
		++SurfaceRegistryVersion;
	}
}

// Removes surfaces when the owning actor/component leaves the world.
void ULSVisionSubsystem::UnregisterSurface(ULSVisionSurfaceComponent* Surface)
{
	if (RegisteredSurfaces.Remove(Surface) > 0)
	{
		++SurfaceRegistryVersion;
	}
}

// Tracks visibility targets that can be shown/hidden by local vision checks.
void ULSVisionSubsystem::RegisterTarget(ULSVisionTargetComponent* Target)
{
	if (Target != nullptr)
	{
		RegisteredTargets.AddUnique(Target);
	}
}

// Removes visibility targets that should no longer receive local visibility updates.
void ULSVisionSubsystem::UnregisterTarget(ULSVisionTargetComponent* Target)
{
	RegisteredTargets.Remove(Target);
}

// Returns only the cached occluder segments near the viewer instead of making every solver traverse the whole world.
void ULSVisionSubsystem::QuerySegmentsInRadius(const FVector2D& Origin, const float Radius, TArray<FLSVisionSegment2D>& OutSegments) const
{
	OutSegments.Reset();

	if (Radius <= 0.0f || GridCellSize <= 0.0f)
	{
		return;
	}

	TArray<FLSVisionGridCellKey> QueryCells;
	CollectGridCellsForBounds(
		FBox2D(Origin - FVector2D(Radius, Radius), Origin + FVector2D(Radius, Radius)),
		QueryCells);

	TSet<int32> UniqueSegmentIds;
	const float RadiusSquared = FMath::Square(Radius);

	for (const FLSVisionGridCellKey& CellKey : QueryCells)
	{
		const FLSVisionGridCell* GridCell = GridCells.Find(CellKey);
		if (GridCell == nullptr)
		{
			continue;
		}

		for (const int32 SegmentId : GridCell->SegmentIds)
		{
			//이미 포함된 세그먼트는 스킵
			if (UniqueSegmentIds.Contains(SegmentId))
			{
				continue;
			}

			//세그먼트 ID로 캐시된 세그먼트 정보 없으면 스킵
			const FLSVisionCachedSegment* CachedSegment = CachedSegments.Find(SegmentId);
			if (CachedSegment == nullptr)
			{
				continue;
			}

			//범위안에 없으면 스킵
			if (ComputeSquaredDistanceToBounds(Origin, CachedSegment->Bounds) > RadiusSquared)
			{
				continue;
			}

			UniqueSegmentIds.Add(SegmentId);
			// CachedSegments(TMap) 내부를 가리키는 포인터가 아니라 값을 복사한다. 포인터를 넘기면
			// 이후 오클루더 등록/해제로 캐시가 재배치될 때 호출자 쪽에서 무효 포인터가 된다.
			OutSegments.Add(CachedSegment->Segment);
		}
	}
}

// Chooses either a configured render target asset or a transient fallback created at runtime.
UTextureRenderTarget2D* ULSVisionSubsystem::ResolveVisibilityMaskRenderTarget()
{
	const ULSVisionSettings* VisionSettings = GetDefault<ULSVisionSettings>();
	if (VisionSettings != nullptr && !VisionSettings->VisibilityMaskRenderTarget.IsNull())
	{
		if (UTextureRenderTarget2D* ConfiguredRenderTarget = VisionSettings->VisibilityMaskRenderTarget.LoadSynchronous())
		{
			// 벽 머티리얼이 이 에셋을 파라미터가 아니라 직접 참조하므로, world별 복사본을 만들면 아무도 안 보는 RT에 쓰게 된다.
			// 따라서 에셋 자체에 쓴다(프로세스 전역 — 동시 활성 world 경고는 Initialize 참고).
			EnforceVisionMaskContractOnConfiguredAsset(*ConfiguredRenderTarget);
			return ConfiguredRenderTarget;
		}

		UE_LOG(LogLS, Warning, TEXT("Failed to load the configured vision mask render target asset. Falling back to transient RT."));
	}

	const int32 FallbackSize = VisionSettings != nullptr
		? FMath::Clamp(VisionSettings->FallbackRenderTargetSize, 128, 4096)
		: 1024;

	return CreateFallbackRenderTarget(FallbackSize);
}

// Creates a transient UAV-capable render target so the shader path works without BP setup.
UTextureRenderTarget2D* ULSVisionSubsystem::CreateFallbackRenderTarget(const int32 Size)
{
	UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>(this, TEXT("LSVisionMaskRT_Transient"));
	if (RenderTarget == nullptr)
	{
		UE_LOG(LogLS, Warning, TEXT("Failed to allocate fallback vision mask render target."));
		return nullptr;
	}

	RenderTarget->RenderTargetFormat = RTF_RGBA8;
	RenderTarget->ClearColor = FLinearColor::Black;
	ApplyVisionMaskRenderTargetContract(*RenderTarget);
	RenderTarget->InitAutoFormat(Size, Size);
	RenderTarget->UpdateResourceImmediate(true);
	return RenderTarget;
}

FLSVisionGridCellKey ULSVisionSubsystem::WorldToGridCell(const FVector2D& Position) const
{
	const float SafeCellSize = FMath::Max(GridCellSize, 1.0f);
	return FLSVisionGridCellKey(
		FMath::FloorToInt(Position.X / SafeCellSize),
		FMath::FloorToInt(Position.Y / SafeCellSize));
}

void ULSVisionSubsystem::CollectGridCellsForBounds(const FBox2D& Bounds, TArray<FLSVisionGridCellKey>& OutCells) const
{
	OutCells.Reset();

	const FLSVisionGridCellKey MinCell = WorldToGridCell(Bounds.Min);
	const FLSVisionGridCellKey MaxCell = WorldToGridCell(Bounds.Max);

	OutCells.Reserve((MaxCell.X - MinCell.X + 1) * (MaxCell.Y - MinCell.Y + 1));

	for (int32 CellX = MinCell.X; CellX <= MaxCell.X; ++CellX)
	{
		for (int32 CellY = MinCell.Y; CellY <= MaxCell.Y; ++CellY)
		{
			OutCells.Add(FLSVisionGridCellKey(CellX, CellY));
		}
	}
}

void ULSVisionSubsystem::RegisterOccluderInGrid(ULSVisionOccluderComponent* Occluder)
{
	if (Occluder == nullptr)
	{
		return;
	}

	UnregisterOccluderFromGrid(Occluder);

	FLSVisionOccluderGridState GridState;
	TSet<FLSVisionGridCellKey> UniqueOccupiedCells;
	TArray<FLSVisionGridCellKey> SegmentCells;

	for (const FLSVisionSegment2D& Segment : Occluder->GetSegments())
	{
		const FVector2D SegmentMin(
			FMath::Min(Segment.Start.X, Segment.End.X),
			FMath::Min(Segment.Start.Y, Segment.End.Y));
		const FVector2D SegmentMax(
			FMath::Max(Segment.Start.X, Segment.End.X),
			FMath::Max(Segment.Start.Y, Segment.End.Y));

		const FBox2D SegmentBounds(SegmentMin, SegmentMax);
		CollectGridCellsForBounds(SegmentBounds, SegmentCells);

		const int32 SegmentId = NextSegmentId++;
		FLSVisionCachedSegment& CachedSegment = CachedSegments.Add(SegmentId);
		CachedSegment.SegmentId = SegmentId;
		CachedSegment.Owner = Occluder;
		CachedSegment.Segment = Segment;
		CachedSegment.Bounds = SegmentBounds;

		GridState.SegmentIds.Add(SegmentId);

		for (const FLSVisionGridCellKey& CellKey : SegmentCells)
		{
			GridCells.FindOrAdd(CellKey).SegmentIds.Add(SegmentId);
			UniqueOccupiedCells.Add(CellKey);
		}
	}

	GridState.OccupiedCells.Reserve(UniqueOccupiedCells.Num());
	for (const FLSVisionGridCellKey& CellKey : UniqueOccupiedCells)
	{
		GridState.OccupiedCells.Add(CellKey);
	}
	OccluderGridStates.Add(Occluder, MoveTemp(GridState));

	++SegmentTopologyVersion;
}

void ULSVisionSubsystem::UnregisterOccluderFromGrid(ULSVisionOccluderComponent* Occluder)
{
	if (Occluder == nullptr)
	{
		return;
	}

	FLSVisionOccluderGridState GridState;
	if (!OccluderGridStates.RemoveAndCopyValue(Occluder, GridState))
	{
		return;
	}

	++SegmentTopologyVersion;

	for (const FLSVisionGridCellKey& CellKey : GridState.OccupiedCells)
	{
		FLSVisionGridCell* GridCell = GridCells.Find(CellKey);
		if (GridCell == nullptr)
		{
			continue;
		}

		GridCell->SegmentIds.RemoveAll(
			[&GridState](const int32 SegmentId)
			{
				return GridState.SegmentIds.Contains(SegmentId);
			});

		if (GridCell->SegmentIds.Num() == 0)
		{
			GridCells.Remove(CellKey);
		}
	}

	for (const int32 SegmentId : GridState.SegmentIds)
	{
		CachedSegments.Remove(SegmentId);
	}
}
