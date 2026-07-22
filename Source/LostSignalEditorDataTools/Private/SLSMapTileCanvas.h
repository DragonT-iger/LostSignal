#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"

class FLSMapTileEditorModel;

DECLARE_DELEGATE_TwoParams(FOnLSMapTilePresetPicked, int32, float);

class SLSMapTileCanvas : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SLSMapTileCanvas) {}
		SLATE_ARGUMENT(TSharedPtr<FLSMapTileEditorModel>, Model)
		SLATE_EVENT(FOnLSMapTilePresetPicked, OnPresetPicked)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void FitToView();

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& KeyEvent) override;
	virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;
	virtual void OnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override;

private:
	FVector2D GetMapOrigin(const FGeometry& Geometry) const;
	FVector2D CellToLocal(const FGeometry& Geometry, const FIntPoint& Cell) const;
	TOptional<FIntPoint> LocalToCell(const FGeometry& Geometry, const FVector2D& LocalPosition) const;
	void UpdatePaintAtPosition(const FGeometry& Geometry, const FVector2D& LocalPosition);
	void PaintBrushToCell(const FIntPoint& Cell);
	void ApplyRectanglePaint();
	bool IsCellInRectangle(const FIntPoint& Cell) const;
	void DrawCell(const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FIntPoint& Cell) const;
	void DrawEmptyMessage(const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;
	float GetCellPixelSize() const;

	TSharedPtr<FLSMapTileEditorModel> Model;
	FOnLSMapTilePresetPicked OnPresetPicked;
	TOptional<FIntPoint> HoveredCell;
	TOptional<FIntPoint> LastPaintedCell;
	TOptional<FIntPoint> RectangleStartCell;
	TOptional<FIntPoint> RectangleEndCell;
	FVector2D PanOffset = FVector2D::ZeroVector;
	FVector2D LastMousePosition = FVector2D::ZeroVector;
	float Zoom = 1.0f;
	bool bPainting = false;
	bool bPanning = false;
};
