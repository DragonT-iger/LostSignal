#include "Vision/LSVisionSubsystem.h"

#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "LostSignal.h"
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
		}
	}

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

	RuntimeMaskRenderTarget = nullptr;

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
		if (const UTextureRenderTarget2D* ConfiguredRenderTarget = VisionSettings->VisibilityMaskRenderTarget.LoadSynchronous())
		{
			if (UTextureRenderTarget2D* RuntimeRenderTarget = CreateRenderTargetFromTemplate(ConfiguredRenderTarget))
			{
				return RuntimeRenderTarget;
			}

			UE_LOG(LogLS, Warning, TEXT("Failed to create runtime vision mask render target from configured asset. Falling back to transient RT."));
		}
	}

	const int32 FallbackSize = VisionSettings != nullptr
		? FMath::Clamp(VisionSettings->FallbackRenderTargetSize, 128, 4096)
		: 1024;

	return CreateFallbackRenderTarget(FallbackSize);
}

// Creates a per-world runtime RT so PIE worlds and listen-server views do not overwrite the same asset.
UTextureRenderTarget2D* ULSVisionSubsystem::CreateRenderTargetFromTemplate(const UTextureRenderTarget2D* TemplateRenderTarget)
{
	if (TemplateRenderTarget == nullptr)
	{
		return nullptr;
	}

	UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>(this, TEXT("LSVisionMaskRT_Runtime"));
	if (RenderTarget == nullptr)
	{
		UE_LOG(LogLS, Warning, TEXT("Failed to allocate runtime vision mask render target from template."));
		return nullptr;
	}

	// 아트 소유값(색·감마·포맷·크기)은 템플릿에서 그대로 상속한다.
	RenderTarget->ClearColor = TemplateRenderTarget->ClearColor;
	RenderTarget->TargetGamma = TemplateRenderTarget->TargetGamma;

	// 주소 모드·mip은 계약이므로 에셋 값을 무시하고 강제한다. 무시된 항목이 있으면 아트가 에셋을 고칠 수 있게 남긴다.
	const bool bTemplateAddressMismatch =
		TemplateRenderTarget->AddressX != TextureAddress::TA_Clamp || TemplateRenderTarget->AddressY != TextureAddress::TA_Clamp;
	if (bTemplateAddressMismatch || TemplateRenderTarget->bAutoGenerateMips)
	{
		UE_LOG(LogLS, Warning,
			TEXT("Vision mask RT 템플릿 '%s'이 계약과 다릅니다(무시하고 강제 적용): AddressX/Y=%d/%d(요구 TA_Clamp), bAutoGenerateMips=%s(요구 false). 에셋을 수정해 주세요."),
			*GetNameSafe(TemplateRenderTarget),
			static_cast<int32>(TemplateRenderTarget->AddressX),
			static_cast<int32>(TemplateRenderTarget->AddressY),
			TemplateRenderTarget->bAutoGenerateMips ? TEXT("true") : TEXT("false"));
	}

	ApplyVisionMaskRenderTargetContract(*RenderTarget);

	if (TemplateRenderTarget->OverrideFormat != PF_Unknown)
	{
		RenderTarget->InitCustomFormat(
			TemplateRenderTarget->SizeX,
			TemplateRenderTarget->SizeY,
			TemplateRenderTarget->OverrideFormat,
			TemplateRenderTarget->bForceLinearGamma);
	}
	else
	{
		RenderTarget->RenderTargetFormat = TemplateRenderTarget->RenderTargetFormat;
		RenderTarget->InitAutoFormat(TemplateRenderTarget->SizeX, TemplateRenderTarget->SizeY);
	}

	RenderTarget->UpdateResourceImmediate(true);
	return RenderTarget;
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
