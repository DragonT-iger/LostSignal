#include "SLSMapTileCanvas.h"

#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"
#include "LSMapTileEditorModel.h"
#include "Styling/AppStyle.h"

namespace
{
	constexpr float GLSMapTileCanvasBaseCellPixels = 22.0f;
}

void SLSMapTileCanvas::Construct(const FArguments& InArgs)
{
	Model = InArgs._Model;
	OnPresetPicked = InArgs._OnPresetPicked;
}

void SLSMapTileCanvas::FitToView()
{
	if (!Model.IsValid() || !Model->HasCells())
	{
		return;
	}

	const FVector2D Size = GetCachedGeometry().GetLocalSize();
	const FIntPoint CellCount = Model->GetMaxCell() - Model->GetMinCell() + FIntPoint(1, 1);
	const float FitX = Size.X / FMath::Max(1.0f, CellCount.X * GLSMapTileCanvasBaseCellPixels);
	const float FitY = Size.Y / FMath::Max(1.0f, CellCount.Y * GLSMapTileCanvasBaseCellPixels);
	Zoom = FMath::Clamp(FMath::Min(FitX, FitY) * 0.9f, 0.2f, 4.0f);
	PanOffset = FVector2D::ZeroVector;
	Invalidate(EInvalidateWidgetReason::Paint);
}

FVector2D SLSMapTileCanvas::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	return FVector2D(720.0f, 720.0f);
}

int32 SLSMapTileCanvas::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(),
		FAppStyle::GetBrush("Brushes.Panel"),
		ESlateDrawEffect::None,
		FLinearColor(0.025f, 0.03f, 0.04f, 1.0f));

	if (!Model.IsValid() || !Model->HasCells())
	{
		DrawEmptyMessage(AllottedGeometry, OutDrawElements, LayerId + 1);
		return LayerId + 1;
	}

	for (int32 Y = Model->GetMinCell().Y; Y <= Model->GetMaxCell().Y; ++Y)
	{
		for (int32 X = Model->GetMinCell().X; X <= Model->GetMaxCell().X; ++X)
		{
			DrawCell(AllottedGeometry, OutDrawElements, LayerId + 1, FIntPoint(X, Y));
		}
	}
	return LayerId + 2;
}

FReply SLSMapTileCanvas::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const FVector2D LocalPosition = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	if (MouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton)
	{
		bPanning = true;
		LastMousePosition = LocalPosition;
		return FReply::Handled().CaptureMouse(SharedThis(this));
	}

	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton || !Model.IsValid())
	{
		return FReply::Unhandled();
	}

	const TOptional<FIntPoint> Cell = LocalToCell(MyGeometry, LocalPosition);
	if (MouseEvent.IsAltDown() && Cell.IsSet())
	{
		OnPresetPicked.ExecuteIfBound(
			Model->PickCellPaletteIndex(Cell.GetValue()),
			Model->GetCellYawDegrees(Cell.GetValue()));
		return FReply::Handled();
	}

	bPainting = Model->BeginPaintStroke();
	LastPaintedCell.Reset();
	RectangleStartCell.Reset();
	RectangleEndCell.Reset();
	UpdatePaintAtPosition(MyGeometry, LocalPosition);
	return bPainting ? FReply::Handled().CaptureMouse(SharedThis(this)) : FReply::Handled();
}

FReply SLSMapTileCanvas::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton && bPanning)
	{
		bPanning = false;
		return FReply::Handled().ReleaseMouseCapture();
	}

	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bPainting)
	{
		if (Model->GetPaintTool() == ELSMapTilePaintTool::Rectangle)
		{
			ApplyRectanglePaint();
		}
		bPainting = false;
		LastPaintedCell.Reset();
		RectangleStartCell.Reset();
		RectangleEndCell.Reset();
		Model->EndPaintStroke();
		Invalidate(EInvalidateWidgetReason::Paint);
		return FReply::Handled().ReleaseMouseCapture();
	}
	return FReply::Unhandled();
}

FReply SLSMapTileCanvas::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const FVector2D LocalPosition = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	const TOptional<FIntPoint> NewHoveredCell = LocalToCell(MyGeometry, LocalPosition);
	if (HoveredCell != NewHoveredCell)
	{
		HoveredCell = NewHoveredCell;
		if (Model.IsValid())
		{
			Model->SetHoveredCell(HoveredCell);
		}
	}
	if (bPanning)
	{
		PanOffset += LocalPosition - LastMousePosition;
		LastMousePosition = LocalPosition;
		Invalidate(EInvalidateWidgetReason::Paint);
		return FReply::Handled();
	}

	if (bPainting && MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		UpdatePaintAtPosition(MyGeometry, LocalPosition);
		return FReply::Handled();
	}
	Invalidate(EInvalidateWidgetReason::Paint);
	return FReply::Unhandled();
}

FReply SLSMapTileCanvas::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	Zoom = FMath::Clamp(Zoom * FMath::Pow(1.15f, MouseEvent.GetWheelDelta()), 0.2f, 4.0f);
	Invalidate(EInvalidateWidgetReason::Paint);
	return FReply::Handled();
}

FReply SLSMapTileCanvas::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent)
{
	if (KeyEvent.GetKey() == EKeys::R && Model.IsValid())
	{
		Model->SetPaintYawDegrees(Model->GetPaintYawDegrees() + 90.0f);
		Invalidate(EInvalidateWidgetReason::Paint);
		return FReply::Handled();
	}
	return SLeafWidget::OnKeyDown(MyGeometry, KeyEvent);
}

void SLSMapTileCanvas::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	SLeafWidget::OnMouseEnter(MyGeometry, MouseEvent);
	FSlateApplication::Get().SetKeyboardFocus(SharedThis(this), EFocusCause::Mouse);
}

void SLSMapTileCanvas::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	HoveredCell.Reset();
	if (Model.IsValid())
	{
		Model->SetHoveredCell(HoveredCell);
	}
	SLeafWidget::OnMouseLeave(MouseEvent);
}

void SLSMapTileCanvas::OnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	if (bPainting && Model.IsValid())
	{
		Model->EndPaintStroke();
	}
	bPainting = false;
	bPanning = false;
	LastPaintedCell.Reset();
	RectangleStartCell.Reset();
	RectangleEndCell.Reset();
	Invalidate(EInvalidateWidgetReason::Paint);
}

FVector2D SLSMapTileCanvas::GetMapOrigin(const FGeometry& Geometry) const
{
	const FIntPoint CellCount = Model->GetMaxCell() - Model->GetMinCell() + FIntPoint(1, 1);
	const FVector2D MapSize(CellCount.X * GetCellPixelSize(), CellCount.Y * GetCellPixelSize());
	return (Geometry.GetLocalSize() - MapSize) * 0.5f + PanOffset;
}

FVector2D SLSMapTileCanvas::CellToLocal(const FGeometry& Geometry, const FIntPoint& Cell) const
{
	const int32 Column = Cell.X - Model->GetMinCell().X;
	const int32 Row = Model->GetMaxCell().Y - Cell.Y;
	return GetMapOrigin(Geometry) + FVector2D(Column, Row) * GetCellPixelSize();
}

TOptional<FIntPoint> SLSMapTileCanvas::LocalToCell(
	const FGeometry& Geometry,
	const FVector2D& LocalPosition) const
{
	if (!Model.IsValid() || !Model->HasCells())
	{
		return TOptional<FIntPoint>();
	}

	const FVector2D Relative = LocalPosition - GetMapOrigin(Geometry);
	const int32 Column = FMath::FloorToInt(Relative.X / GetCellPixelSize());
	const int32 Row = FMath::FloorToInt(Relative.Y / GetCellPixelSize());
	const FIntPoint Cell(Model->GetMinCell().X + Column, Model->GetMaxCell().Y - Row);
	if (Cell.X < Model->GetMinCell().X || Cell.X > Model->GetMaxCell().X
		|| Cell.Y < Model->GetMinCell().Y || Cell.Y > Model->GetMaxCell().Y)
	{
		return TOptional<FIntPoint>();
	}
	return Cell;
}

void SLSMapTileCanvas::UpdatePaintAtPosition(const FGeometry& Geometry, const FVector2D& LocalPosition)
{
	const TOptional<FIntPoint> Cell = LocalToCell(Geometry, LocalPosition);
	if (!Cell.IsSet() || !Model->HasCell(Cell.GetValue()))
	{
		return;
	}

	if (Model->GetPaintTool() == ELSMapTilePaintTool::Rectangle)
	{
		if (!RectangleStartCell.IsSet())
		{
			RectangleStartCell = Cell;
		}
		RectangleEndCell = Cell;
	}
	else
	{
		PaintBrushToCell(Cell.GetValue());
	}
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SLSMapTileCanvas::PaintBrushToCell(const FIntPoint& Cell)
{
	if (!LastPaintedCell.IsSet())
	{
		Model->PaintCell(Cell);
		LastPaintedCell = Cell;
		return;
	}
	if (LastPaintedCell.GetValue() == Cell)
	{
		return;
	}

	const FIntPoint Start = LastPaintedCell.GetValue();
	const int32 StepCount = FMath::Max(FMath::Abs(Cell.X - Start.X), FMath::Abs(Cell.Y - Start.Y));
	for (int32 Step = 1; Step <= StepCount; ++Step)
	{
		const float Alpha = static_cast<float>(Step) / StepCount;
		const FIntPoint PaintCell(
			FMath::RoundToInt(FMath::Lerp(static_cast<float>(Start.X), static_cast<float>(Cell.X), Alpha)),
			FMath::RoundToInt(FMath::Lerp(static_cast<float>(Start.Y), static_cast<float>(Cell.Y), Alpha)));
		if (Model->HasCell(PaintCell))
		{
			Model->PaintCell(PaintCell);
		}
	}
	LastPaintedCell = Cell;
}

void SLSMapTileCanvas::ApplyRectanglePaint()
{
	if (!RectangleStartCell.IsSet() || !RectangleEndCell.IsSet())
	{
		return;
	}

	const FIntPoint MinCell(
		FMath::Min(RectangleStartCell->X, RectangleEndCell->X),
		FMath::Min(RectangleStartCell->Y, RectangleEndCell->Y));
	const FIntPoint MaxCell(
		FMath::Max(RectangleStartCell->X, RectangleEndCell->X),
		FMath::Max(RectangleStartCell->Y, RectangleEndCell->Y));
	for (int32 Y = MinCell.Y; Y <= MaxCell.Y; ++Y)
	{
		for (int32 X = MinCell.X; X <= MaxCell.X; ++X)
		{
			const FIntPoint Cell(X, Y);
			if (Model->HasCell(Cell))
			{
				Model->PaintCell(Cell);
			}
		}
	}
}

bool SLSMapTileCanvas::IsCellInRectangle(const FIntPoint& Cell) const
{
	if (!bPainting || !RectangleStartCell.IsSet() || !RectangleEndCell.IsSet())
	{
		return false;
	}

	return Cell.X >= FMath::Min(RectangleStartCell->X, RectangleEndCell->X)
		&& Cell.X <= FMath::Max(RectangleStartCell->X, RectangleEndCell->X)
		&& Cell.Y >= FMath::Min(RectangleStartCell->Y, RectangleEndCell->Y)
		&& Cell.Y <= FMath::Max(RectangleStartCell->Y, RectangleEndCell->Y);
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
		Geometry.ToPaintGeometry(FVector2D(CellSize, CellSize), FSlateLayoutTransform(Position)),
		FAppStyle::GetBrush("WhiteBrush"),
		ESlateDrawEffect::None,
		BorderColor);

	const FVector2D InnerPosition = Position + FVector2D(1.0f);
	const FVector2D InnerSize(FMath::Max(1.0f, CellSize - 2.0f));
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		LayerId + 1,
		Geometry.ToPaintGeometry(InnerSize, FSlateLayoutTransform(InnerPosition)),
		FAppStyle::GetBrush("WhiteBrush"),
		ESlateDrawEffect::None,
		Model->HasCell(Cell) ? Model->GetCellColor(Cell) : FLinearColor(0.035f, 0.04f, 0.05f, 1.0f));
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
	return GLSMapTileCanvasBaseCellPixels * Zoom;
}
