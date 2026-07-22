#include "LSMapTileEditorModel.h"

#include "Editor.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "LSMapTilePalette.h"
#include "Materials/MaterialInterface.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "LSMapTileEditorModel"

namespace
{
	const FName GLSMapTileEditorActorTag(TEXT("LS.MapTile"));
}

FLSMapTileEditorModel::~FLSMapTileEditorModel()
{
	ClearHoverPreview();
}

void FLSMapTileEditorModel::SetPalette(ULSMapTilePalette* InPalette)
{
	Palette = InPalette;
	ActivePaletteIndex = INDEX_NONE;
	RefreshFromEditorWorld();
}

void FLSMapTileEditorModel::SetActivePaletteIndex(int32 InPaletteIndex)
{
	const ULSMapTilePalette* TilePalette = Palette.Get();
	ActivePaletteIndex = TilePalette != nullptr && TilePalette->Tiles.IsValidIndex(InPaletteIndex)
		? InPaletteIndex
		: INDEX_NONE;
	RefreshHoverPreview();
}

void FLSMapTileEditorModel::SetGridSize(float InGridSize)
{
	GridSize = FMath::Max(InGridSize, 1.0f);
}

void FLSMapTileEditorModel::SetAlignTopSurface(bool bInAlignTopSurface)
{
	bAlignTopSurface = bInAlignTopSurface;
	RefreshHoverPreview();
}

void FLSMapTileEditorModel::SetPaintYawDegrees(float InYawDegrees)
{
	PaintYawDegrees = FMath::Fmod(InYawDegrees, 360.0f);
	if (PaintYawDegrees < 0.0f)
	{
		PaintYawDegrees += 360.0f;
	}
	RefreshHoverPreview();
}

void FLSMapTileEditorModel::SetHoveredCell(const TOptional<FIntPoint>& InHoveredCell)
{
	if (HoveredCell == InHoveredCell)
	{
		return;
	}
	HoveredCell = InHoveredCell;
	RefreshHoverPreview();
}

void FLSMapTileEditorModel::RefreshFromEditorWorld()
{
	ClearHoverPreview();
	Cells.Reset();
	UWorld* World = GetEditorWorld();
	if (World == nullptr)
	{
		SetStatus(LOCTEXT("WorldRequiredStatus", "현재 에디터 레벨을 찾을 수 없습니다."));
		return;
	}

	for (AStaticMeshActor* Actor : FindFixedTileActors(World))
	{
		AddActorCell(Actor);
	}

	UpdateBounds();
	SetStatus(Cells.IsEmpty()
		? FText::Format(LOCTEXT("NoFixedTilesStatus", "현재 레벨에서 {0} 간격의 반복 바닥 타일을 찾지 못했습니다."), GridSize)
		: FText::Format(LOCTEXT("RefreshStatus", "현재 레벨의 고정 바닥 타일 {0}개를 불러왔습니다."), Cells.Num()));
	RefreshHoverPreview();
}

float FLSMapTileEditorModel::GetCellYawDegrees(const FIntPoint& Cell) const
{
	const FLSMapTileEditorCell* FoundCell = Cells.Find(Cell);
	const AStaticMeshActor* Actor = FoundCell != nullptr ? FoundCell->Actor.Get() : nullptr;
	return Actor != nullptr ? Actor->GetActorRotation().Yaw : 0.0f;
}

FLinearColor FLSMapTileEditorModel::GetCellColor(const FIntPoint& Cell) const
{
	const FLSMapTileEditorCell* FoundCell = Cells.Find(Cell);
	AStaticMeshActor* Actor = FoundCell != nullptr ? FoundCell->Actor.Get() : nullptr;
	UStaticMeshComponent* Component = Actor != nullptr ? Actor->GetStaticMeshComponent() : nullptr;
	const int32 PaletteIndex = FindMatchingPaletteIndex(Component);
	if (const ULSMapTilePalette* TilePalette = Palette.Get(); TilePalette != nullptr && TilePalette->Tiles.IsValidIndex(PaletteIndex))
	{
		return TilePalette->Tiles[PaletteIndex].PreviewColor;
	}
	return Component != nullptr ? FLinearColor(0.22f, 0.22f, 0.24f, 1.0f) : FLinearColor(0.08f, 0.08f, 0.09f, 1.0f);
}

bool FLSMapTileEditorModel::BeginPaintStroke()
{
	if (GetActiveTile() == nullptr)
	{
		SetStatus(LOCTEXT("ActiveTileRequiredStatus", "팔레트에서 배치할 타일을 선택하세요."));
		return false;
	}

	if (ActiveTransaction.IsValid())
	{
		EndPaintStroke();
	}
	ActiveTransaction = MakeUnique<FScopedTransaction>(LOCTEXT("PaintTilesTransaction", "2D 맵 타일 칠하기"));
	StrokeChangeCount = 0;
	return true;
}

bool FLSMapTileEditorModel::PaintCell(const FIntPoint& Cell)
{
	FLSMapTileEditorCell* FoundCell = Cells.Find(Cell);
	AStaticMeshActor* Actor = FoundCell != nullptr ? FoundCell->Actor.Get() : nullptr;
	if (!ActiveTransaction.IsValid() || Actor == nullptr)
	{
		return false;
	}

	if (ReplaceCellMesh(Actor))
	{
		++StrokeChangeCount;
		return true;
	}
	return false;
}

void FLSMapTileEditorModel::EndPaintStroke()
{
	if (!ActiveTransaction.IsValid())
	{
		return;
	}

	if (StrokeChangeCount == 0)
	{
		ActiveTransaction->Cancel();
	}
	ActiveTransaction.Reset();
	SetStatus(FText::Format(LOCTEXT("PaintedStatus", "타일 {0}개를 교체했습니다."), StrokeChangeCount));
	GEditor->RedrawLevelEditingViewports();
}

int32 FLSMapTileEditorModel::PickCellPaletteIndex(const FIntPoint& Cell) const
{
	const FLSMapTileEditorCell* FoundCell = Cells.Find(Cell);
	AStaticMeshActor* Actor = FoundCell != nullptr ? FoundCell->Actor.Get() : nullptr;
	return FindMatchingPaletteIndex(Actor != nullptr ? Actor->GetStaticMeshComponent() : nullptr);
}

UWorld* FLSMapTileEditorModel::GetEditorWorld() const
{
	return GEditor != nullptr ? GEditor->GetEditorWorldContext().World() : nullptr;
}

const FLSMapTilePaletteEntry* FLSMapTileEditorModel::GetActiveTile() const
{
	const ULSMapTilePalette* TilePalette = Palette.Get();
	return TilePalette != nullptr && TilePalette->Tiles.IsValidIndex(ActivePaletteIndex)
		? &TilePalette->Tiles[ActivePaletteIndex]
		: nullptr;
}

int32 FLSMapTileEditorModel::FindMatchingPaletteIndex(const UStaticMeshComponent* Component) const
{
	const ULSMapTilePalette* TilePalette = Palette.Get();
	if (TilePalette == nullptr || Component == nullptr)
	{
		return INDEX_NONE;
	}

	return TilePalette->Tiles.IndexOfByPredicate([this, Component](const FLSMapTilePaletteEntry& Tile)
	{
		return DoesComponentMatchTile(Component, Tile);
	});
}

bool FLSMapTileEditorModel::DoesComponentMatchTile(
	const UStaticMeshComponent* Component,
	const FLSMapTilePaletteEntry& Tile) const
{
	if (Component == nullptr || Tile.Mesh == nullptr || Component->GetStaticMesh() != Tile.Mesh)
	{
		return false;
	}

	const int32 MaterialCount = Tile.Materials.IsEmpty()
		? Tile.Mesh->GetStaticMaterials().Num()
		: Tile.Materials.Num();
	if (Component->GetNumMaterials() != MaterialCount)
	{
		return false;
	}

	for (int32 Index = 0; Index < MaterialCount; ++Index)
	{
		UMaterialInterface* ExpectedMaterial = Tile.Materials.IsEmpty()
			? Tile.Mesh->GetMaterial(Index)
			: Tile.Materials[Index].Get();
		if (Component->GetMaterial(Index) != ExpectedMaterial)
		{
			return false;
		}
	}
	return true;
}

bool FLSMapTileEditorModel::IsGridAligned(const AStaticMeshActor* Actor) const
{
	if (Actor == nullptr)
	{
		return false;
	}

	const FVector Location = Actor->GetActorLocation();
	const FIntPoint Cell = GetCellCoordinate(Actor);
	const double SnappedX = GridOrigin.X + Cell.X * GridSize;
	const double SnappedY = GridOrigin.Y + Cell.Y * GridSize;
	return FMath::IsNearlyEqual(Location.X, SnappedX, 1.0)
		&& FMath::IsNearlyEqual(Location.Y, SnappedY, 1.0);
}

FIntPoint FLSMapTileEditorModel::GetCellCoordinate(const AStaticMeshActor* Actor) const
{
	const FVector Location = Actor->GetActorLocation();
	return FIntPoint(
		FMath::RoundToInt((Location.X - GridOrigin.X) / GridSize),
		FMath::RoundToInt((Location.Y - GridOrigin.Y) / GridSize));
}

TArray<AStaticMeshActor*> FLSMapTileEditorModel::FindFixedTileActors(UWorld* World) const
{
	TMap<UStaticMesh*, TArray<AStaticMeshActor*>> ActorsByMesh;
	TArray<AStaticMeshActor*> TaggedActors;
	for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
	{
		UStaticMeshComponent* Component = It->GetStaticMeshComponent();
		if (Component == nullptr || Component->GetStaticMesh() == nullptr || !IsGridAligned(*It))
		{
			continue;
		}

		if (It->ActorHasTag(GLSMapTileEditorActorTag))
		{
			TaggedActors.Add(*It);
		}
		else
		{
			ActorsByMesh.FindOrAdd(Component->GetStaticMesh()).Add(*It);
		}
	}

	TArray<AStaticMeshActor*> FixedTileActors;
	for (const TPair<UStaticMesh*, TArray<AStaticMeshActor*>>& Pair : ActorsByMesh)
	{
		if (Pair.Value.Num() > FixedTileActors.Num())
		{
			FixedTileActors = Pair.Value;
		}
	}
	FixedTileActors.Append(TaggedActors);
	return FixedTileActors;
}

void FLSMapTileEditorModel::AddActorCell(AStaticMeshActor* Actor)
{
	const FIntPoint Coordinate = GetCellCoordinate(Actor);
	FLSMapTileEditorCell* ExistingCell = Cells.Find(Coordinate);
	if (ExistingCell != nullptr && ExistingCell->Actor.IsValid()
		&& ExistingCell->Actor->ActorHasTag(GLSMapTileEditorActorTag))
	{
		return;
	}

	FLSMapTileEditorCell& Cell = Cells.FindOrAdd(Coordinate);
	Cell.Coordinate = Coordinate;
	Cell.Actor = Actor;
}

bool FLSMapTileEditorModel::ReplaceCellMesh(AStaticMeshActor* Actor)
{
	UStaticMeshComponent* Component = Actor != nullptr ? Actor->GetStaticMeshComponent() : nullptr;
	const FLSMapTilePaletteEntry* ActiveTile = GetActiveTile();
	const bool bRotationMatches = Actor != nullptr
		&& FMath::IsNearlyEqual(FRotator::NormalizeAxis(Actor->GetActorRotation().Yaw), FRotator::NormalizeAxis(PaintYawDegrees), 0.01f);
	if (Component == nullptr || ActiveTile == nullptr || ActiveTile->Mesh == nullptr
		|| (DoesComponentMatchTile(Component, *ActiveTile) && bRotationMatches))
	{
		return false;
	}

	const double OldTopZ = Component->CalcBounds(Component->GetComponentTransform()).GetBox().Max.Z;
	Actor->Modify();
	Component->Modify();
	Component->SetStaticMesh(ActiveTile->Mesh.Get());
	Component->EmptyOverrideMaterials();
	for (int32 Index = 0; Index < ActiveTile->Materials.Num(); ++Index)
	{
		Component->SetMaterial(Index, ActiveTile->Materials[Index].Get());
	}
	Actor->Tags.AddUnique(GLSMapTileEditorActorTag);
	FRotator TargetRotation = Actor->GetActorRotation();
	TargetRotation.Yaw = PaintYawDegrees;
	Actor->SetActorRotation(TargetRotation);

	if (bAlignTopSurface)
	{
		const double NewTopZ = Component->CalcBounds(Component->GetComponentTransform()).GetBox().Max.Z;
		Actor->SetActorLocation(Actor->GetActorLocation() + FVector(0.0, 0.0, OldTopZ - NewTopZ));
		Actor->PostEditMove(true);
	}
	Actor->MarkPackageDirty();
	RefreshHoverPreview();
	return true;
}

void FLSMapTileEditorModel::UpdateBounds()
{
	if (Cells.IsEmpty())
	{
		MinCell = FIntPoint::ZeroValue;
		MaxCell = FIntPoint::ZeroValue;
		return;
	}

	MinCell = FIntPoint(MAX_int32, MAX_int32);
	MaxCell = FIntPoint(MIN_int32, MIN_int32);
	for (const TPair<FIntPoint, FLSMapTileEditorCell>& Pair : Cells)
	{
		MinCell.X = FMath::Min(MinCell.X, Pair.Key.X);
		MinCell.Y = FMath::Min(MinCell.Y, Pair.Key.Y);
		MaxCell.X = FMath::Max(MaxCell.X, Pair.Key.X);
		MaxCell.Y = FMath::Max(MaxCell.Y, Pair.Key.Y);
	}
}

#undef LOCTEXT_NAMESPACE
