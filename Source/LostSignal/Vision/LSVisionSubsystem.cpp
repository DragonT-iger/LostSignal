#include "Vision/LSVisionSubsystem.h"

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
void ULSVisionSubsystem::RegisterSurface(ULSVisionSurfaceComponent* Surface)
{
	if (Surface != nullptr)
	{
		RegisteredSurfaces.AddUnique(Surface);
	}
}

// Removes surfaces when the owning actor/component leaves the world.
void ULSVisionSubsystem::UnregisterSurface(ULSVisionSurfaceComponent* Surface)
{
	RegisteredSurfaces.Remove(Surface);
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
void ULSVisionSubsystem::QuerySegmentsInRadius(const FVector2D& Origin, const float Radius, TArray<FLSVisionSegment2D*>& OutSegments) const
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
			if (UniqueSegmentIds.Contains(SegmentId))
			{
				continue;
			}

			const FLSVisionCachedSegment* CachedSegment = CachedSegments.Find(SegmentId);
			if (CachedSegment == nullptr)
			{
				continue;
			}

			if (ComputeSquaredDistanceToBounds(Origin, CachedSegment->Bounds) > RadiusSquared)
			{
				continue;
			}

			UniqueSegmentIds.Add(SegmentId);
			OutSegments.Add(const_cast<FLSVisionSegment2D*>(&CachedSegment->Segment));
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

	RenderTarget->ClearColor = TemplateRenderTarget->ClearColor;
	RenderTarget->bAutoGenerateMips = TemplateRenderTarget->bAutoGenerateMips;
	RenderTarget->bCanCreateUAV = true;
	RenderTarget->AddressX = TemplateRenderTarget->AddressX;
	RenderTarget->AddressY = TemplateRenderTarget->AddressY;
	RenderTarget->TargetGamma = TemplateRenderTarget->TargetGamma;

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
	RenderTarget->bAutoGenerateMips = false;
	RenderTarget->bCanCreateUAV = true;
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
