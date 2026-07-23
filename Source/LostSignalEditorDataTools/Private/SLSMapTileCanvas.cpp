#include "SLSMapTileCanvas.h"

#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"
#include "LSMapTileEditorModel.h"
#include "Styling/AppStyle.h"

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

	const FVector2D Size = GetCachedGeometry().GetLocalSize() - FVector2D(RulerSizePixels);
	const FIntPoint CellCount = Model->GetMaxCell() - Model->GetMinCell() + FIntPoint(1, 1);
	const float FitX = Size.X / FMath::Max(1.0f, CellCount.X * BaseCellPixels);
	const float FitY = Size.Y / FMath::Max(1.0f, CellCount.Y * BaseCellPixels);
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

	const FVector2D ContentSize = AllottedGeometry.GetLocalSize() - FVector2D(RulerSizePixels);
	const FGeometry ContentGeometry = AllottedGeometry.MakeChild(
		ContentSize.ComponentMax(FVector2D(1.0f)),
		FSlateLayoutTransform(FVector2D(RulerSizePixels)));
	OutDrawElements.PushClip(FSlateClippingZone(ContentGeometry.GetLayoutBoundingRect()));
	for (int32 Y = Model->GetMinCell().Y; Y <= Model->GetMaxCell().Y; ++Y)
	{
		for (int32 X = Model->GetMinCell().X; X <= Model->GetMaxCell().X; ++X)
		{
			DrawCell(AllottedGeometry, OutDrawElements, LayerId + 1, FIntPoint(X, Y));
		}
	}
	OutDrawElements.PopClip();
	DrawRulers(AllottedGeometry, OutDrawElements, LayerId + 4);
	return LayerId + 5;
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

FSlateRect SLSMapTileCanvas::GetContentRect(const FGeometry& Geometry) const
{
	const FVector2D Size = Geometry.GetLocalSize();
	return FSlateRect(RulerSizePixels, RulerSizePixels, Size.X, Size.Y);
}

FVector2D SLSMapTileCanvas::GetMapOrigin(const FGeometry& Geometry) const
{
	const FIntPoint CellCount = Model->GetMaxCell() - Model->GetMinCell() + FIntPoint(1, 1);
	const FVector2D MapSize(CellCount.X * GetCellPixelSize(), CellCount.Y * GetCellPixelSize());
	const FVector2D ContentSize = Geometry.GetLocalSize() - FVector2D(RulerSizePixels);
	return FVector2D(RulerSizePixels) + (ContentSize - MapSize) * 0.5f + PanOffset;
}

FVector2D SLSMapTileCanvas::CellToLocal(const FGeometry& Geometry, const FIntPoint& Cell) const
{
	const int32 Column = Cell.X - Model->GetMinCell().X;
	const int32 Row = Cell.Y - Model->GetMinCell().Y;
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
	if (!GetContentRect(Geometry).ContainsPoint(LocalPosition))
	{
		return TOptional<FIntPoint>();
	}

	const FVector2D Relative = LocalPosition - GetMapOrigin(Geometry);
	const int32 Column = FMath::FloorToInt(Relative.X / GetCellPixelSize());
	const int32 Row = FMath::FloorToInt(Relative.Y / GetCellPixelSize());
	const FIntPoint Cell(Model->GetMinCell().X + Column, Model->GetMinCell().Y + Row);
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
		Model->FocusViewportOnSelection(RectangleStartCell.GetValue(), RectangleEndCell.GetValue());
	}
	else
	{
		PaintBrushToCell(Cell.GetValue());
		Model->FocusViewportOnSelection(Cell.GetValue(), Cell.GetValue());
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
