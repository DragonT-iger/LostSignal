#pragma once

#include "CoreMinimal.h"
#include "LSMapTilePalette.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SCompoundWidget.h"

class FAssetThumbnail;
class FAssetThumbnailPool;
class FLSMapTileEditorModel;
class SLSMapTileCanvas;
struct FAssetData;

struct FLSMapTilePaletteListItem
{
	int32 PaletteIndex = INDEX_NONE;
	TSharedPtr<FAssetThumbnail> Thumbnail;
};

class SLSMapTileEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLSMapTileEditor) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	using FPaletteItemPtr = TSharedPtr<FLSMapTilePaletteListItem>;

	TSharedRef<SWidget> BuildToolbar();
	TSharedRef<SWidget> BuildPalettePanel();
	TSharedRef<SWidget> BuildSettingsPanel();
	TSharedRef<ITableRow> GeneratePaletteRow(FPaletteItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable);
	void HandlePaletteSelection(FPaletteItemPtr Item, ESelectInfo::Type SelectInfo);
	void HandleCanvasPresetPicked(int32 PaletteIndex, float YawDegrees);
	void SetPalette(ULSMapTilePalette* InPalette);
	void RebuildPaletteItems();
	void RefreshMap();
	FString GetPaletteAssetPath() const;
	void OnPaletteAssetChanged(const FAssetData& AssetData);
	FReply OnCreatePalette();
	FReply OnAddSelectedActors();
	FReply OnRemoveSelectedTile();
	FReply OnRefreshMap();
	FReply OnFitMap();
	void CollectSelectedTilePresets(TArray<FLSMapTilePaletteEntry>& OutPresets) const;
	bool AddTilePresetToPalette(ULSMapTilePalette& TilePalette, const FLSMapTilePaletteEntry& Preset) const;
	FLinearColor MakePreviewColor(const FLSMapTilePaletteEntry& Preset) const;

	TSharedPtr<FLSMapTileEditorModel> Model;
	TSharedPtr<SLSMapTileCanvas> Canvas;
	TSharedPtr<SListView<FPaletteItemPtr>> PaletteListView;
	TArray<FPaletteItemPtr> PaletteItems;
	TSharedPtr<FAssetThumbnailPool> ThumbnailPool;
	TStrongObjectPtr<ULSMapTilePalette> Palette;
};
