#include "SLSMapTileCanvas.h"

#include "AssetThumbnail.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "LSMapTileEditorModel.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/AppStyle.h"

namespace
{
	constexpr float GLSMapTileGridLinePixels = 1.0f;
	constexpr float GLSMapTileMinimumRulerLabelSpacing = 14.0f;
}

void SLSMapTileCanvas::SetTileThumbnails(TArray<TSharedPtr<FAssetThumbnail>> InTileThumbnails)
{
	TileThumbnails = MoveTemp(InTileThumbnails);
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SLSMapTileCanvas::DrawCell(
	const FGeometry& Geometry,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FIntPoint& Cell) const
{
	const FVector2D Position = CellToLocal(Geometry, Cell);
	const float CellSize = GetCellPixelSize();
	const bool bHovered = HoveredCell.IsSet() && HoveredCell.GetValue() == Cell;
	const bool bInRectangle = IsCellInRectangle(Cell);
	const FLinearColor BorderColor = bInRectangle
		? FLinearColor(0.12f, 0.72f, 1.0f, 1.0f)
		: bHovered ? FLinearColor(1.0f, 0.82f, 0.18f, 1.0f) : FLinearColor(0.08f, 0.09f, 0.11f, 1.0f);
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		Geometry.ToPaintGeometry(FVector2D(CellSize), FSlateLayoutTransform(Position)),
		FAppStyle::GetBrush("WhiteBrush"),
		ESlateDrawEffect::None,
		BorderColor);

	const FVector2D InnerPosition = Position + FVector2D(GLSMapTileGridLinePixels);
	const FVector2D InnerSize(FMath::Max(1.0f, CellSize - GLSMapTileGridLinePixels * 2.0f));
	const FGeometry InnerGeometry = Geometry.MakeChild(InnerSize, FSlateLayoutTransform(InnerPosition));
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId + 1,
		InnerGeometry.ToPaintGeometry(),
		FAppStyle::GetBrush("WhiteBrush"),
		ESlateDrawEffect::None,
		Model->HasCell(Cell) ? Model->GetCellColor(Cell) : FLinearColor(0.035f, 0.04f, 0.05f, 1.0f));

	const bool bShowPaintPreview = bHovered && Model->GetActivePaletteIndex() != INDEX_NONE;
	const TSharedPtr<FAssetThumbnail> Thumbnail = bShowPaintPreview
		? GetActiveTileThumbnail()
		: GetCellThumbnail(Cell);
	if (!Thumbnail.IsValid())
	{
		return;
	}

	const float PreviewYaw = bShowPaintPreview ? Model->GetPaintYawDegrees() : Model->GetCellYawDegrees(Cell);
	const float RotationRadians = FMath::DegreesToRadians(PreviewYaw);
	const FGeometry RotatedGeometry = Geometry.MakeChild(
		InnerSize,
		FSlateLayoutTransform(InnerPosition),
		FSlateRenderTransform(FQuat2D(RotationRadians)),
		FVector2D(0.5f));
	OutDrawElements.PushClip(FSlateClippingZone(InnerGeometry.GetLayoutBoundingRect()));
	FSlateDrawElement::MakeViewport(
		OutDrawElements,
		LayerId + 2,
		RotatedGeometry.ToPaintGeometry(),
		Thumbnail,
		ESlateDrawEffect::None,
		FLinearColor::White);
	OutDrawElements.PopClip();
}

TSharedPtr<FAssetThumbnail> SLSMapTileCanvas::GetCellThumbnail(const FIntPoint& Cell) const
{
	const int32 PaletteIndex = Model.IsValid() ? Model->PickCellPaletteIndex(Cell) : INDEX_NONE;
	return TileThumbnails.IsValidIndex(PaletteIndex) ? TileThumbnails[PaletteIndex] : nullptr;
}

TSharedPtr<FAssetThumbnail> SLSMapTileCanvas::GetActiveTileThumbnail() const
{
	const int32 PaletteIndex = Model.IsValid() ? Model->GetActivePaletteIndex() : INDEX_NONE;
	return TileThumbnails.IsValidIndex(PaletteIndex) ? TileThumbnails[PaletteIndex] : nullptr;
}

void SLSMapTileCanvas::DrawRulers(
	const FGeometry& Geometry,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId) const
{
	const FVector2D Size = Geometry.GetLocalSize();
	const FLinearColor RulerColor(0.045f, 0.052f, 0.065f, 1.0f);
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		Geometry.ToPaintGeometry(FVector2D(Size.X, RulerSizePixels), FSlateLayoutTransform()),
		FAppStyle::GetBrush("WhiteBrush"),
		ESlateDrawEffect::None,
		RulerColor);
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		Geometry.ToPaintGeometry(FVector2D(RulerSizePixels, Size.Y), FSlateLayoutTransform()),
		FAppStyle::GetBrush("WhiteBrush"),
		ESlateDrawEffect::None,
		RulerColor);

	const FSlateFontInfo Font = FAppStyle::GetFontStyle("SmallFont");
	const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const FSlateRect ContentRect = GetContentRect(Geometry);
	const float CellSize = GetCellPixelSize();
	const int32 LabelStep = FMath::Max(1, FMath::CeilToInt(GLSMapTileMinimumRulerLabelSpacing / CellSize));
	const FLinearColor NormalColor(0.72f, 0.76f, 0.82f, 1.0f);
	const FLinearColor HoverColor(1.0f, 0.82f, 0.18f, 1.0f);

	for (int32 X = Model->GetMinCell().X; X <= Model->GetMaxCell().X; ++X)
	{
		const int32 DisplayColumn = X - Model->GetMinCell().X + 1;
		if ((DisplayColumn - 1) % LabelStep != 0 && X != Model->GetMaxCell().X)
		{
			continue;
		}
		const float CenterX = CellToLocal(Geometry, FIntPoint(X, Model->GetMaxCell().Y)).X + CellSize * 0.5f;
		if (CenterX < ContentRect.Left || CenterX > ContentRect.Right)
		{
			continue;
		}
		const FText Label = FText::AsNumber(DisplayColumn);
		const FVector2D TextSize = FontMeasure->Measure(Label, Font);
		const bool bHoveredColumn = HoveredCell.IsSet() && HoveredCell->X == X;
		FSlateDrawElement::MakeText(
			OutDrawElements,
			LayerId + 1,
			Geometry.ToPaintGeometry(TextSize, FSlateLayoutTransform(FVector2D(CenterX - TextSize.X * 0.5f, (RulerSizePixels - TextSize.Y) * 0.5f))),
			Label,
			Font,
			ESlateDrawEffect::None,
			bHoveredColumn ? HoverColor : NormalColor);
	}

	for (int32 Y = Model->GetMinCell().Y; Y <= Model->GetMaxCell().Y; ++Y)
	{
		const int32 DisplayRow = Y - Model->GetMinCell().Y + 1;
		if ((DisplayRow - 1) % LabelStep != 0 && Y != Model->GetMaxCell().Y)
		{
			continue;
		}
		const float CenterY = CellToLocal(Geometry, FIntPoint(Model->GetMinCell().X, Y)).Y + CellSize * 0.5f;
		if (CenterY < ContentRect.Top || CenterY > ContentRect.Bottom)
		{
			continue;
		}
		const FText Label = FText::AsNumber(DisplayRow);
		const FVector2D TextSize = FontMeasure->Measure(Label, Font);
		const bool bHoveredRow = HoveredCell.IsSet() && HoveredCell->Y == Y;
		FSlateDrawElement::MakeText(
			OutDrawElements,
			LayerId + 1,
			Geometry.ToPaintGeometry(TextSize, FSlateLayoutTransform(FVector2D((RulerSizePixels - TextSize.X) * 0.5f, CenterY - TextSize.Y * 0.5f))),
			Label,
			Font,
			ESlateDrawEffect::None,
			bHoveredRow ? HoverColor : NormalColor);
	}
}

void SLSMapTileCanvas::DrawEmptyMessage(
	const FGeometry& Geometry,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId) const
{
	FSlateDrawElement::MakeText(
		OutDrawElements,
		LayerId,
		Geometry.ToPaintGeometry(FVector2D(520.0f, 30.0f), FSlateLayoutTransform(FVector2D(24.0f))),
		Model.IsValid() ? Model->GetStatusText() : FText::GetEmpty(),
		FAppStyle::GetFontStyle("NormalFont"),
		ESlateDrawEffect::None,
		FLinearColor::White);
}

float SLSMapTileCanvas::GetCellPixelSize() const
{
	return BaseCellPixels * Zoom;
}
