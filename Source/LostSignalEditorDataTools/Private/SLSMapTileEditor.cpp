#include "SLSMapTileEditor.h"

#include "AssetRegistry/AssetData.h"
#include "AssetThumbnail.h"
#include "AssetToolsModule.h"
#include "Engine/StaticMesh.h"
#include "Factories/DataAssetFactory.h"
#include "LSMapTileEditorModel.h"
#include "LSMapTilePalette.h"
#include "Materials/MaterialInterface.h"
#include "Modules/ModuleManager.h"
#include "PropertyCustomizationHelpers.h"
#include "ScopedTransaction.h"
#include "SLSMapTileCanvas.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SLSMapTileEditor"

void SLSMapTileEditor::Construct(const FArguments& InArgs)
{
	Model = MakeShared<FLSMapTileEditorModel>();
	ThumbnailPool = MakeShared<FAssetThumbnailPool>(64);

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[BuildToolbar()]
		+ SVerticalBox::Slot().FillHeight(1.0f)
		[
			SNew(SSplitter)
			+ SSplitter::Slot().Value(0.22f)[BuildPalettePanel()]
			+ SSplitter::Slot().Value(0.60f)
			[
				SAssignNew(Canvas, SLSMapTileCanvas)
				.Model(Model)
				.OnPresetPicked(this, &SLSMapTileEditor::HandleCanvasPresetPicked)
			]
			+ SSplitter::Slot().Value(0.18f)[BuildSettingsPanel()]
		]
	];
}

TSharedRef<SWidget> SLSMapTileEditor::BuildToolbar()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(6.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(LOCTEXT("PaletteLabel", "타일 팔레트"))
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(6.0f, 0.0f)
			[
				SNew(SObjectPropertyEntryBox)
				.AllowedClass(ULSMapTilePalette::StaticClass())
				.ObjectPath(this, &SLSMapTileEditor::GetPaletteAssetPath)
				.OnObjectChanged(this, &SLSMapTileEditor::OnPaletteAssetChanged)
				.DisplayBrowse(true)
				.DisplayUseSelected(true)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f, 0.0f)
			[
				SNew(SButton).Text(LOCTEXT("CreatePaletteButton", "새 팔레트")).OnClicked(this, &SLSMapTileEditor::OnCreatePalette)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(8.0f, 0.0f, 2.0f, 0.0f)
			[
				SNew(SButton).Text(LOCTEXT("RefreshButton", "현재 레벨 새로고침")).OnClicked(this, &SLSMapTileEditor::OnRefreshMap)
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SButton).Text(LOCTEXT("FitButton", "전체 보기")).OnClicked(this, &SLSMapTileEditor::OnFitMap)
			]
		];
}

TSharedRef<SWidget> SLSMapTileEditor::BuildPalettePanel()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
		.Padding(6.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("PalettePanelTitle", "등록된 타일"))
				.Font(FAppStyle::GetFontStyle("DetailsView.CategoryFontStyle"))
			]
			+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 6.0f)
			[
				SAssignNew(PaletteListView, SListView<FPaletteItemPtr>)
				.ListItemsSource(&PaletteItems)
				.OnGenerateRow(this, &SLSMapTileEditor::GeneratePaletteRow)
				.OnSelectionChanged(this, &SLSMapTileEditor::HandlePaletteSelection)
				.SelectionMode(ESelectionMode::Single)
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SButton)
				.Text(LOCTEXT("AddSelectedActorsButton", "선택 액터 외형 등록"))
				.ToolTipText(LOCTEXT("AddSelectedActorsTooltip", "레벨에서 선택한 액터의 메시와 전체 머티리얼 슬롯을 타일 프리셋으로 추가합니다."))
				.OnClicked(this, &SLSMapTileEditor::OnAddSelectedActors)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 0.0f)
			[
				SNew(SButton).Text(LOCTEXT("RemoveTileButton", "선택 타일 제거")).OnClicked(this, &SLSMapTileEditor::OnRemoveSelectedTile)
			]
		];
}

TSharedRef<ITableRow> SLSMapTileEditor::GeneratePaletteRow(
	FPaletteItemPtr Item,
	const TSharedRef<STableViewBase>& OwnerTable)
{
	const ULSMapTilePalette* TilePalette = Palette.Get();
	const FLSMapTilePaletteEntry* Entry = TilePalette != nullptr && TilePalette->Tiles.IsValidIndex(Item->PaletteIndex)
		? &TilePalette->Tiles[Item->PaletteIndex]
		: nullptr;
	const FText DisplayName = Entry != nullptr && !Entry->DisplayName.IsEmpty()
		? Entry->DisplayName
		: FText::FromString(Entry != nullptr && !Entry->Materials.IsEmpty() && Entry->Materials[0] != nullptr
			? Entry->Materials[0]->GetName()
			: Entry != nullptr && Entry->Mesh != nullptr ? Entry->Mesh->GetName() : TEXT("None"));

	return SNew(STableRow<FPaletteItemPtr>, OwnerTable)
		.Padding(3.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).WidthOverride(56.0f).HeightOverride(56.0f)
				[
					Item->Thumbnail.IsValid() ? Item->Thumbnail->MakeThumbnailWidget() : SNullWidget::NullWidget
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(8.0f, 0.0f)
			[
				SNew(STextBlock).Text(DisplayName).AutoWrapText(true)
			]
		];
}

void SLSMapTileEditor::HandlePaletteSelection(FPaletteItemPtr Item, ESelectInfo::Type SelectInfo)
{
	ULSMapTilePalette* TilePalette = Palette.Get();
	const int32 PaletteIndex = TilePalette != nullptr && Item.IsValid() && TilePalette->Tiles.IsValidIndex(Item->PaletteIndex)
		? Item->PaletteIndex
		: INDEX_NONE;
	Model->SetActivePaletteIndex(PaletteIndex);
}

void SLSMapTileEditor::HandleCanvasPresetPicked(int32 PaletteIndex, float YawDegrees)
{
	Model->SetPaintYawDegrees(YawDegrees);
	if (!Palette.IsValid() || !Palette->Tiles.IsValidIndex(PaletteIndex))
	{
		return;
	}

	for (const FPaletteItemPtr& Item : PaletteItems)
	{
		if (Item.IsValid() && Item->PaletteIndex == PaletteIndex)
		{
			PaletteListView->SetSelection(Item);
			PaletteListView->RequestScrollIntoView(Item);
			return;
		}
	}
}

void SLSMapTileEditor::SetPalette(ULSMapTilePalette* InPalette)
{
	Palette.Reset(InPalette);
	Model->SetPalette(InPalette);
	RebuildPaletteItems();
	if (Canvas.IsValid())
	{
		Canvas->FitToView();
	}
}

void SLSMapTileEditor::RebuildPaletteItems()
{
	PaletteItems.Reset();
	if (ULSMapTilePalette* TilePalette = Palette.Get())
	{
		for (int32 Index = 0; Index < TilePalette->Tiles.Num(); ++Index)
		{
			FPaletteItemPtr Item = MakeShared<FLSMapTilePaletteListItem>();
			Item->PaletteIndex = Index;
			const FLSMapTilePaletteEntry& Entry = TilePalette->Tiles[Index];
			UObject* ThumbnailAsset = Entry.Mesh.Get();
			for (UMaterialInterface* Material : Entry.Materials)
			{
				if (Material != nullptr)
				{
					ThumbnailAsset = Material;
					break;
				}
			}
			if (ThumbnailAsset != nullptr)
			{
				Item->Thumbnail = MakeShared<FAssetThumbnail>(ThumbnailAsset, 56, 56, ThumbnailPool);
			}
			PaletteItems.Add(Item);
		}
	}
	if (PaletteListView.IsValid())
	{
		PaletteListView->RequestListRefresh();
	}
}

void SLSMapTileEditor::RefreshMap()
{
	Model->RefreshFromEditorWorld();
	if (Canvas.IsValid())
	{
		Canvas->Invalidate(EInvalidateWidgetReason::Paint);
	}
}

FString SLSMapTileEditor::GetPaletteAssetPath() const
{
	return Palette.IsValid() ? Palette->GetPathName() : FString();
}

void SLSMapTileEditor::OnPaletteAssetChanged(const FAssetData& AssetData)
{
	SetPalette(Cast<ULSMapTilePalette>(AssetData.GetAsset()));
}

FReply SLSMapTileEditor::OnCreatePalette()
{
	UDataAssetFactory* Factory = NewObject<UDataAssetFactory>();
	Factory->DataAssetClass = ULSMapTilePalette::StaticClass();
	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	SetPalette(Cast<ULSMapTilePalette>(AssetToolsModule.Get().CreateAssetWithDialog(ULSMapTilePalette::StaticClass(), Factory)));
	return FReply::Handled();
}

FReply SLSMapTileEditor::OnAddSelectedActors()
{
	ULSMapTilePalette* TilePalette = Palette.Get();
	if (TilePalette == nullptr)
	{
		return FReply::Handled();
	}

	TArray<FLSMapTilePaletteEntry> SelectedPresets;
	CollectSelectedTilePresets(SelectedPresets);
	if (SelectedPresets.IsEmpty())
	{
		return FReply::Handled();
	}

	const FScopedTransaction Transaction(LOCTEXT("AddPaletteTilesTransaction", "팔레트 타일 등록"));
	TilePalette->Modify();
	for (const FLSMapTilePaletteEntry& Preset : SelectedPresets)
	{
		AddTilePresetToPalette(*TilePalette, Preset);
	}
	TilePalette->MarkPackageDirty();
	TilePalette->PostEditChange();
	RebuildPaletteItems();
	RefreshMap();
	return FReply::Handled();
}

FReply SLSMapTileEditor::OnRemoveSelectedTile()
{
	ULSMapTilePalette* TilePalette = Palette.Get();
	const TArray<FPaletteItemPtr> SelectedItems = PaletteListView.IsValid()
		? PaletteListView->GetSelectedItems()
		: TArray<FPaletteItemPtr>();
	FPaletteItemPtr SelectedItem = SelectedItems.IsEmpty() ? nullptr : SelectedItems[0];
	if (TilePalette == nullptr || !SelectedItem.IsValid() || !TilePalette->Tiles.IsValidIndex(SelectedItem->PaletteIndex))
	{
		return FReply::Handled();
	}

	const FScopedTransaction Transaction(LOCTEXT("RemovePaletteTileTransaction", "팔레트 타일 제거"));
	TilePalette->Modify();
	TilePalette->Tiles.RemoveAt(SelectedItem->PaletteIndex);
	TilePalette->MarkPackageDirty();
	TilePalette->PostEditChange();
	Model->SetActivePaletteIndex(INDEX_NONE);
	RebuildPaletteItems();
	RefreshMap();
	return FReply::Handled();
}

FReply SLSMapTileEditor::OnRefreshMap()
{
	RefreshMap();
	return FReply::Handled();
}

FReply SLSMapTileEditor::OnFitMap()
{
	if (Canvas.IsValid())
	{
		Canvas->FitToView();
	}
	return FReply::Handled();
}

FLinearColor SLSMapTileEditor::MakePreviewColor(const FLSMapTilePaletteEntry& Preset) const
{
	uint32 Hash = GetTypeHash(Preset.Mesh != nullptr ? Preset.Mesh->GetPathName() : FString());
	for (const UMaterialInterface* Material : Preset.Materials)
	{
		Hash = HashCombine(Hash, GetTypeHash(Material != nullptr ? Material->GetPathName() : FString()));
	}
	return FLinearColor::MakeFromHSV8(static_cast<uint8>(Hash % 255), 150, 205);
}

#undef LOCTEXT_NAMESPACE
