#pragma once

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CoreMinimal.h"
#include "UObject/StrongObjectPtr.h"

class AStaticMeshActor;
class FScopedTransaction;
class ULSMapTilePalette;
class UWorld;
struct FLSMapTilePaletteEntry;

struct FLSMapTileEditorCell
{
	FIntPoint Coordinate = FIntPoint::ZeroValue;
	TWeakObjectPtr<AStaticMeshActor> Actor;
};

enum class ELSMapTilePaintTool : uint8
{
	Brush,
	Rectangle
};

class FLSMapTileEditorModel
{
public:
	FLSMapTileEditorModel() = default;
	~FLSMapTileEditorModel();

	void SetPalette(ULSMapTilePalette* InPalette);
	void SetActivePaletteIndex(int32 InPaletteIndex);
	void SetGridSize(float InGridSize);
	void SetGridOrigin(const FVector2D& InGridOrigin) { GridOrigin = InGridOrigin; }
	void SetAlignTopSurface(bool bInAlignTopSurface);
	void SetPaintYawDegrees(float InYawDegrees);
	void SetHoveredCell(const TOptional<FIntPoint>& InHoveredCell);
	void SetPaintTool(ELSMapTilePaintTool InPaintTool) { PaintTool = InPaintTool; }

	ULSMapTilePalette* GetPalette() const { return Palette.Get(); }
	int32 GetActivePaletteIndex() const { return ActivePaletteIndex; }
	float GetGridSize() const { return GridSize; }
	const FVector2D& GetGridOrigin() const { return GridOrigin; }
	bool ShouldAlignTopSurface() const { return bAlignTopSurface; }
	float GetPaintYawDegrees() const { return PaintYawDegrees; }
	ELSMapTilePaintTool GetPaintTool() const { return PaintTool; }
	const FText& GetStatusText() const { return StatusText; }

	void RefreshFromEditorWorld();
	bool HasCells() const { return !Cells.IsEmpty(); }
	bool HasCell(const FIntPoint& Cell) const { return Cells.Contains(Cell); }
	FIntPoint GetMinCell() const { return MinCell; }
	FIntPoint GetMaxCell() const { return MaxCell; }
	FLinearColor GetCellColor(const FIntPoint& Cell) const;
	float GetCellYawDegrees(const FIntPoint& Cell) const;

	bool BeginPaintStroke();
	bool PaintCell(const FIntPoint& Cell);
	void EndPaintStroke();
	int32 PickCellPaletteIndex(const FIntPoint& Cell) const;

private:
	UWorld* GetEditorWorld() const;
	const FLSMapTilePaletteEntry* GetActiveTile() const;
	int32 FindMatchingPaletteIndex(const UStaticMeshComponent* Component) const;
	bool DoesComponentMatchTile(const UStaticMeshComponent* Component, const FLSMapTilePaletteEntry& Tile) const;
	bool IsGridAligned(const AStaticMeshActor* Actor) const;
	FIntPoint GetCellCoordinate(const AStaticMeshActor* Actor) const;
	TArray<AStaticMeshActor*> FindFixedTileActors(UWorld* World) const;
	void AddActorCell(AStaticMeshActor* Actor);
	bool ReplaceCellMesh(AStaticMeshActor* Actor);
	void RefreshHoverPreview();
	void ClearHoverPreview();
	void UpdateBounds();
	void SetStatus(const FText& InStatus) { StatusText = InStatus; }

	TWeakObjectPtr<ULSMapTilePalette> Palette;
	int32 ActivePaletteIndex = INDEX_NONE;
	TMap<FIntPoint, FLSMapTileEditorCell> Cells;
	TUniquePtr<FScopedTransaction> ActiveTransaction;
	TStrongObjectPtr<UStaticMeshComponent> HoverPreviewComponent;
	TStrongObjectPtr<UBoxComponent> HoverHighlightComponent;
	TOptional<FIntPoint> HoveredCell;
	FIntPoint MinCell = FIntPoint::ZeroValue;
	FIntPoint MaxCell = FIntPoint::ZeroValue;
	FVector2D GridOrigin = FVector2D::ZeroVector;
	float GridSize = 800.0f;
	float PaintYawDegrees = 0.0f;
	ELSMapTilePaintTool PaintTool = ELSMapTilePaintTool::Brush;
	int32 StrokeChangeCount = 0;
	bool bAlignTopSurface = true;
	FText StatusText;
};
