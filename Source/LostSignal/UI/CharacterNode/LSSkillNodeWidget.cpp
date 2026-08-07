#include "UI/CharacterNode/LSSkillNodeWidget.h"

#include "InputCoreTypes.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

namespace
{
// 파일 로컬 헬퍼에는 SkillNodeShape 접두사를 붙인다(유니티 빌드 중복 정의 방지).

// 중심에서 12시 방향을 첫 꼭짓점으로 하는 정다각형. 변 수를 늘리면 원이 된다.
void SkillNodeShapeMakeRegularPolygon(const FVector2D& Center, const float Radius, const int32 SideCount, TArray<FVector2D>& OutPoints)
{
	OutPoints.Reset(SideCount);
	for (int32 Index = 0; Index < SideCount; ++Index)
	{
		const float Angle = (2.0f * UE_PI) * static_cast<float>(Index) / static_cast<float>(SideCount);
		OutPoints.Add(Center + FVector2D(FMath::Sin(Angle), -FMath::Cos(Angle)) * Radius);
	}
}

// 볼록 다각형 채우기. 가로 띠를 MakeBox 로 쌓는다(ULSMinimapWidget::DrawFilledCircle 과 같은 방식).
// Slate 에 다각형 채우기 프리미티브가 없어서 이 방법을 쓴다.
void SkillNodeShapeDrawFilledPolygon(
	FSlateWindowElementList& OutDrawElements,
	const int32 LayerId,
	const FGeometry& Geometry,
	const TArray<FVector2D>& Points,
	const FLinearColor& Color)
{
	if (Points.Num() < 3)
	{
		return;
	}

	const FSlateBrush* Brush = FCoreStyle::Get().GetBrush("WhiteBrush");
	constexpr float ScanStep = 2.0f;

	float MinY = Points[0].Y;
	float MaxY = Points[0].Y;
	for (const FVector2D& Point : Points)
	{
		MinY = FMath::Min(MinY, Point.Y);
		MaxY = FMath::Max(MaxY, Point.Y);
	}

	for (float Y = MinY; Y < MaxY; Y += ScanStep)
	{
		const float RowHeight = FMath::Min(ScanStep, MaxY - Y);
		const float ScanY = Y + RowHeight * 0.5f;

		float MinX = TNumericLimits<float>::Max();
		float MaxX = TNumericLimits<float>::Lowest();
		for (int32 Index = 0; Index < Points.Num(); ++Index)
		{
			const FVector2D& Start = Points[Index];
			const FVector2D& End = Points[(Index + 1) % Points.Num()];
			const float Height = End.Y - Start.Y;
			if (FMath::IsNearlyZero(Height))
			{
				continue;
			}

			const float Alpha = (ScanY - Start.Y) / Height;
			if (Alpha < 0.0f || Alpha > 1.0f)
			{
				continue;
			}

			const float X = Start.X + (End.X - Start.X) * Alpha;
			MinX = FMath::Min(MinX, X);
			MaxX = FMath::Max(MaxX, X);
		}

		if (MaxX <= MinX)
		{
			continue;
		}

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			Geometry.ToPaintGeometry(
				FVector2f(static_cast<float>(MaxX - MinX), RowHeight),
				FSlateLayoutTransform(FVector2f(static_cast<float>(MinX), Y))),
			Brush,
			ESlateDrawEffect::None,
			Color);
	}
}
} // namespace

void ULSSkillNodeWidget::SetNodeView(const FLSSkillNodeView& InView)
{
	NodeView = InView;
	Invalidate(EInvalidateWidgetReason::Paint);
}

void ULSSkillNodeWidget::SetSelected(const bool bInSelected)
{
	if (bIsSelected == bInSelected)
	{
		return;
	}

	bIsSelected = bInSelected;
	Invalidate(EInvalidateWidgetReason::Paint);
}

float ULSSkillNodeWidget::GetShapeRadius() const
{
	switch (NodeView.Kind)
	{
	case ELSSkillNodeKind::Core:
		return CoreRadius;
	case ELSSkillNodeKind::MainStat:
		return MainStatRadius;
	case ELSSkillNodeKind::SubStat:
		return SubStatRadius;
	case ELSSkillNodeKind::SkillEnhance:
		return EnhanceRadius;
	case ELSSkillNodeKind::SkillEvolve:
		return EvolveRadius;
	default:
		return SubStatRadius;
	}
}

float ULSSkillNodeWidget::GetDesiredSlotExtent() const
{
	return GetShapeRadius() * 2.0f + FMath::Max(OutlineThickness, SelectedOutlineThickness) * 2.0f;
}

int32 ULSSkillNodeWidget::GetShapeSideCount() const
{
	switch (NodeView.Kind)
	{
	case ELSSkillNodeKind::SkillEnhance:
		return 4; // 마름모
	case ELSSkillNodeKind::SkillEvolve:
		return 6; // 육각형
	default:
		return 32; // 원 근사
	}
}

FLinearColor ULSSkillNodeWidget::GetFillColor() const
{
	switch (NodeView.State)
	{
	case ELSSkillNodeState::Activated:
		return ActivatedFillColor;
	case ELSSkillNodeState::Available:
		return AvailableFillColor;
	default:
		return LockedFillColor;
	}
}

FLinearColor ULSSkillNodeWidget::GetOutlineColor() const
{
	if (bIsSelected)
	{
		return SelectedOutlineColor;
	}
	if (bIsPointerOver)
	{
		return HoveredOutlineColor;
	}

	switch (NodeView.State)
	{
	case ELSSkillNodeState::Activated:
		return ActivatedOutlineColor;
	case ELSSkillNodeState::Available:
		return AvailableOutlineColor;
	default:
		return LockedOutlineColor;
	}
}

int32 ULSSkillNodeWidget::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	const int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	const bool bParentEnabled) const
{
	if (NodeView.Kind == ELSSkillNodeKind::None)
	{
		return Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	}

	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	const FVector2D Center = LocalSize * 0.5f;
	const float Radius = GetShapeRadius();

	TArray<FVector2D> ShapePoints;
	SkillNodeShapeMakeRegularPolygon(Center, Radius, GetShapeSideCount(), ShapePoints);

	SkillNodeShapeDrawFilledPolygon(OutDrawElements, LayerId, AllottedGeometry, ShapePoints, GetFillColor());

	// 외곽선은 닫힌 폴리라인이다. 첫 점을 끝에 한 번 더 넣어야 마지막 변이 그려진다.
	TArray<FVector2D> OutlinePoints = ShapePoints;
	OutlinePoints.Add(ShapePoints[0]);
	const float Thickness = bIsSelected ? SelectedOutlineThickness : OutlineThickness;
	if (Thickness > 0.0f)
	{
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId + 1,
			AllottedGeometry.ToPaintGeometry(),
			OutlinePoints,
			ESlateDrawEffect::None,
			GetOutlineColor(),
			true,
			Thickness);
	}

	if (bDrawDebugKey)
	{
		FSlateDrawElement::MakeText(
			OutDrawElements,
			LayerId + 2,
			AllottedGeometry.ToPaintGeometry(
				FVector2f(static_cast<float>(LocalSize.X), 12.0f),
				FSlateLayoutTransform(FVector2f(0.0f, static_cast<float>(Center.Y) - 6.0f))),
			FText::FromName(NodeView.NodeKey),
			FCoreStyle::GetDefaultFontStyle("Regular", 8),
			ESlateDrawEffect::None,
			FLinearColor::White);
	}

	const int32 MaxLayer = Super::NativePaint(
		Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId + 3, InWidgetStyle, bParentEnabled);
	return FMath::Max(MaxLayer, LayerId + 3);
}

void ULSSkillNodeWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	if (!bIsPointerOver)
	{
		bIsPointerOver = true;
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}

void ULSSkillNodeWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	if (bIsPointerOver)
	{
		bIsPointerOver = false;
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}

FReply ULSSkillNodeWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && !NodeView.NodeKey.IsNone())
	{
		// 슬롯은 사각형이지만 도형은 원·마름모·육각형이다. 도형 밖(모서리) 클릭은 흘려보낸다.
		const FVector2D Local = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
		const FVector2D Center = InGeometry.GetLocalSize() * 0.5f;
		if (FVector2D::Distance(Local, Center) <= GetShapeRadius())
		{
			OnClicked.Broadcast(this);
			return FReply::Handled();
		}
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}
