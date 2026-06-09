#include "UI/Minimap/LSMinimapWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/MeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Data/LSChipStats.h"
#include "Data/LSGameDataSubsystem.h"
#include "Data/LSProtocolUnlockRow.h"
#include "Engine/GameInstance.h"
#include "Fonts/SlateFontInfo.h"
#include "Internationalization/Text.h"
#include "LostSignal.h"
#include "Minimap/LSMinimapMarkerComponent.h"
#include "Minimap/LSMinimapObstacleComponent.h"
#include "Minimap/LSMinimapShapeActor.h"
#include "Minimap/LSMinimapSubsystem.h"
#include "Session/LSSaveSubsystem.h"
#include "Styling/CoreStyle.h"
#include "Vision/LSVisionOccluderComponent.h"
#include "Vision/LSVisionSurfaceComponent.h"
#include "Vision/LSVisionSubsystem.h"
#include "Vision/LSVisionTypes.h"

namespace
{
constexpr float MinimapPadding = 4.0f;
constexpr float CircleFillStep = 1.0f;

FLinearColor ResolveMarkerColor(const FLSMinimapMarkerSnapshot& Marker)
{
	if (Marker.Color != FLinearColor::White)
	{
		return Marker.Color;
	}

	switch (Marker.MarkerType)
	{
	case ELSMinimapMarkerType::Enemy:
		return FLinearColor(1.0f, 0.12f, 0.1f, 1.0f);
	case ELSMinimapMarkerType::Loot:
		return FLinearColor(1.0f, 0.82f, 0.18f, 1.0f);
	case ELSMinimapMarkerType::DroppedItem:
		return FLinearColor(0.25f, 1.0f, 0.42f, 1.0f);
	case ELSMinimapMarkerType::Extraction:
		return FLinearColor(0.28f, 1.0f, 0.45f, 1.0f);
	default:
		return Marker.Color;
	}
}

int32 ResolveInactiveSignalSlotCount(const float SignalPercent)
{
	const float SignalPercent100 = FMath::Clamp(SignalPercent, 0.0f, 1.0f) * 100.0f;
	return FMath::Clamp(FMath::FloorToInt((100.0f - SignalPercent100 + KINDA_SMALL_NUMBER) / 10.0f), 0, 10);
}

TArray<FLSSessionItem> BuildSignalActiveEquipmentItems(const TArray<FLSSessionItem>& Items, const int32 InactiveSlotCount)
{
	TArray<FLSSessionItem> ActiveItems;
	ActiveItems.Reserve(Items.Num());

	for (int32 SlotIndex = InactiveSlotCount; SlotIndex < Items.Num(); ++SlotIndex)
	{
		ActiveItems.Add(Items[SlotIndex]);
	}

	return ActiveItems;
}
}

void ULSMinimapWidget::InitializeMinimapForPawn(APawn* InPawn)
{
	ObservedPawn = InPawn;
	bUsePreviewNavigationLevels = false;
	if (!InPawn)
	{
		UE_LOG(LogLS, Warning, TEXT("%s cannot initialize minimap because pawn is missing."), *GetNameSafe(this));
	}
}

void ULSMinimapWidget::SetPreviewNavigationLevels(const int32 CurrentNavigationProtocol, const int32 PreviousNavigationProtocol)
{
	bUsePreviewNavigationLevels = true;
	ObservedPawn.Reset();
	PreviewCurrentNavigationProtocol = FMath::Max(0, CurrentNavigationProtocol);
	PreviewPreviousNavigationProtocol = FMath::Max(PreviewCurrentNavigationProtocol, PreviousNavigationProtocol);
	InvalidateLayoutAndVolatility();
}

void ULSMinimapWidget::ClearPreviewNavigationLevels()
{
	bUsePreviewNavigationLevels = false;
	PreviewCurrentNavigationProtocol = 0;
	PreviewPreviousNavigationProtocol = 0;
	InvalidateLayoutAndVolatility();
}

void ULSMinimapWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void ULSMinimapWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	InvalidateLayoutAndVolatility();
}

int32 ULSMinimapWidget::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	const int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	const bool bParentEnabled) const
{
	int32 CurrentLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	const FVector2D Size = AllottedGeometry.GetLocalSize();
	const float Radius = FMath::Max(1.0f, (FMath::Min(Size.X, Size.Y) * 0.5f) - MinimapPadding);
	const FVector2D Center = Size * 0.5f;
	const float PixelsPerCm = Radius / FMath::Max(ViewRadiusCm, 1.0f);

	DrawFilledCircle(OutDrawElements, ++CurrentLayer, AllottedGeometry, Center, Radius, BackgroundColor);
	DrawCircleOutline(OutDrawElements, CurrentLayer, AllottedGeometry, Center, Radius - 0.75f, BackgroundColor, 1.5f);

	int32 CurrentNavigationProtocol = 0;
	int32 PreviousNavigationProtocol = 0;
	ResolveNavigationProtocolLevels(CurrentNavigationProtocol, PreviousNavigationProtocol);
	if (!IsNavigationFeatureVisible(TEXT("Minimap"), CurrentNavigationProtocol, PreviousNavigationProtocol, CurrentNavigationProtocol >= RevealPolicy.LootVisibleNavigation))
	{
		return CurrentLayer;
	}

	if (bUsePreviewNavigationLevels)
	{
		DrawPreviewData(AllottedGeometry, OutDrawElements, CurrentLayer, Center, Radius, CurrentNavigationProtocol, PreviousNavigationProtocol);
		return CurrentLayer;
	}

	UWorld* World = GetWorld();
	ULSMinimapSubsystem* MinimapSubsystem = World ? World->GetSubsystem<ULSMinimapSubsystem>() : nullptr;
	if (MinimapSubsystem)
	{
		TArray<ALSMinimapShapeActor*> Shapes;
		MinimapSubsystem->GetRegisteredShapes(Shapes);
		for (const ALSMinimapShapeActor* Shape : Shapes)
		{
			if (Shape)
			{
				DrawShape(Shape->BuildSnapshot(), AllottedGeometry, OutDrawElements, ++CurrentLayer, Center, Radius, PixelsPerCm);
			}
		}
	}
	DrawVisionTerrain(AllottedGeometry, OutDrawElements, CurrentLayer, Center, Radius, PixelsPerCm);
	DrawMinimapObstacles(AllottedGeometry, OutDrawElements, CurrentLayer, Center, Radius, PixelsPerCm);

	const APawn* Pawn = ObservedPawn.Get();
	if (Pawn && IsNavigationFeatureVisible(TEXT("Minimap_View_Angle"), CurrentNavigationProtocol, PreviousNavigationProtocol, true))
	{
		const FVector2D Forward2D = ProjectWorldDirection(Pawn->GetActorForwardVector());
		DrawSightCone(OutDrawElements, ++CurrentLayer, AllottedGeometry, Center, Forward2D, Radius, SightAngleDegrees, SightColor);
	}

	if (MinimapSubsystem)
	{
		TArray<ULSMinimapMarkerComponent*> Markers;
		MinimapSubsystem->GetRegisteredMarkers(Markers);
		Markers.Sort([](const ULSMinimapMarkerComponent& A, const ULSMinimapMarkerComponent& B)
		{
			return A.BuildSnapshot().Priority < B.BuildSnapshot().Priority;
		});

		for (const ULSMinimapMarkerComponent* MarkerComponent : Markers)
		{
			if (!MarkerComponent)
			{
				continue;
			}

			const FLSMinimapMarkerSnapshot Marker = MarkerComponent->BuildSnapshot();
			const FVector2D Projected = ProjectWorldLocation(Marker.WorldLocation, Center, PixelsPerCm);
			if (!ShouldDrawMarker(Marker, Projected, Center, Radius, CurrentNavigationProtocol, PreviousNavigationProtocol))
			{
				continue;
			}

			const bool bOffscreen = FVector2D::Distance(Projected, Center) > Radius;
			const FVector2D DrawPoint = bOffscreen ? ClampToMinimapEdge(Projected, Center, Radius - 4.0f) : Projected;
			DrawFilledCircle(OutDrawElements, ++CurrentLayer, AllottedGeometry, DrawPoint, Marker.DrawRadius, ResolveMarkerColor(Marker));

			if (Marker.MarkerType == ELSMinimapMarkerType::Extraction && ObservedPawn.IsValid())
			{
				const float DistanceMeters = FVector::Dist2D(ObservedPawn->GetActorLocation(), Marker.WorldLocation) / 100.0f;
				const FText DistanceText = FText::AsNumber(FMath::RoundToInt(DistanceMeters));
				DrawText(OutDrawElements, ++CurrentLayer, AllottedGeometry, DrawPoint + FVector2D(6.0f, -6.0f), DistanceText, ResolveMarkerColor(Marker));
			}
		}
	}

	if (IsNavigationFeatureVisible(TEXT("Player_Point"), CurrentNavigationProtocol, PreviousNavigationProtocol, true))
	{
		DrawFilledCircle(OutDrawElements, ++CurrentLayer, AllottedGeometry, Center, 5.0f, PlayerColor);
	}
	return CurrentLayer;
}

FVector2D ULSMinimapWidget::ProjectWorldLocation(const FVector WorldLocation, const FVector2D& Center, const float PixelsPerCm) const
{
	const APawn* Pawn = ObservedPawn.Get();
	if (!Pawn)
	{
		return Center;
	}

	const FVector Delta = WorldLocation - Pawn->GetActorLocation();
	return Center + ProjectWorldDirection(Delta) * PixelsPerCm;
}

FVector2D ULSMinimapWidget::ProjectWorldDirection(const FVector WorldDirection) const
{
	FVector ViewUp = FVector::ForwardVector;
	FVector ViewRight = FVector::RightVector;
	ResolveMinimapViewAxes(ViewUp, ViewRight);

	const FVector Direction2D = FVector(WorldDirection.X, WorldDirection.Y, 0.0f);
	return FVector2D(
		FVector::DotProduct(Direction2D, ViewRight),
		-FVector::DotProduct(Direction2D, ViewUp));
}

FVector2D ULSMinimapWidget::ClampToMinimapEdge(const FVector2D& Point, const FVector2D& Center, const float Radius) const
{
	const FVector2D Direction = Point - Center;
	if (Direction.IsNearlyZero())
	{
		return Center;
	}

	return Center + Direction.GetSafeNormal() * Radius;
}

bool ULSMinimapWidget::ResolveMinimapViewAxes(FVector& OutViewUp, FVector& OutViewRight) const
{
	const APawn* Pawn = ObservedPawn.Get();
	if (!Pawn)
	{
		return false;
	}

	if (const UCameraComponent* CameraComponent = Pawn->FindComponentByClass<UCameraComponent>())
	{
		OutViewUp = CameraComponent->GetUpVector().GetSafeNormal2D();
		OutViewRight = CameraComponent->GetRightVector().GetSafeNormal2D();
		if (!OutViewUp.IsNearlyZero() && !OutViewRight.IsNearlyZero())
		{
			return true;
		}
	}

	FRotator ViewRotation = FRotator::ZeroRotator;
	if (const AController* Controller = Pawn->GetController())
	{
		FVector ViewLocation = FVector::ZeroVector;
		Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
	}
	else
	{
		ViewRotation = Pawn->GetActorRotation();
	}

	const FRotator YawRotation(0.0f, ViewRotation.Yaw, 0.0f);
	OutViewUp = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	OutViewRight = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	return true;
}

bool ULSMinimapWidget::ShouldDrawMarker(const FLSMinimapMarkerSnapshot& Marker, const FVector2D& ProjectedPoint, const FVector2D& Center, const float Radius, const int32 CurrentNavigationProtocol, const int32 PreviousNavigationProtocol) const
{
	if (!Marker.bVisible || !IsValid(Marker.OwnerActor))
	{
		return false;
	}

	if (Marker.bAlwaysVisible)
	{
		return true;
	}

	switch (Marker.MarkerType)
	{
	case ELSMinimapMarkerType::Enemy:
		return IsNavigationFeatureVisible(
			TEXT("Minimap_Enemy"),
			CurrentNavigationProtocol,
			PreviousNavigationProtocol,
			CurrentNavigationProtocol >= RevealPolicy.EnemyAlwaysVisibleNavigation ||
				(CurrentNavigationProtocol >= RevealPolicy.EnemyVisibleNavigation && IsEnemyInSight(Marker)));
	case ELSMinimapMarkerType::Loot:
	case ELSMinimapMarkerType::DroppedItem:
		return IsNavigationFeatureVisible(TEXT("Minimap_Looting_Object"), CurrentNavigationProtocol, PreviousNavigationProtocol, CurrentNavigationProtocol >= RevealPolicy.LootVisibleNavigation) &&
			FVector2D::Distance(ProjectedPoint, Center) <= Radius;
	case ELSMinimapMarkerType::Extraction:
		return IsNavigationFeatureVisible(TEXT("Exit_Point"), CurrentNavigationProtocol, PreviousNavigationProtocol, CurrentNavigationProtocol >= RevealPolicy.ExtractionVisibleNavigation);
	default:
		return false;
	}
}

bool ULSMinimapWidget::IsEnemyInSight(const FLSMinimapMarkerSnapshot& Marker) const
{
	const APawn* Pawn = ObservedPawn.Get();
	if (!Pawn)
	{
		return false;
	}

	const FVector ToMarker = Marker.WorldLocation - Pawn->GetActorLocation();
	if (ToMarker.Size2D() > ViewRadiusCm)
	{
		return false;
	}

	const FVector Forward = Pawn->GetActorForwardVector().GetSafeNormal2D();
	const FVector Direction = ToMarker.GetSafeNormal2D();
	const float Dot = FVector::DotProduct(Forward, Direction);
	const float MinDot = FMath::Cos(FMath::DegreesToRadians(SightAngleDegrees * 0.5f));
	return Dot >= MinDot;
}

void ULSMinimapWidget::ResolveNavigationProtocolLevels(int32& OutCurrentNavigationProtocol, int32& OutPreviousNavigationProtocol) const
{
	OutCurrentNavigationProtocol = 0;
	OutPreviousNavigationProtocol = 0;

	if (bUsePreviewNavigationLevels)
	{
		OutCurrentNavigationProtocol = PreviewCurrentNavigationProtocol;
		OutPreviousNavigationProtocol = PreviewPreviousNavigationProtocol;
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	const ULSSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
	if (!SaveSubsystem)
	{
		return;
	}

	const int32 InactiveSlotCount = ResolveInactiveSignalSlotCount(SaveSubsystem->GetChipSignalGaugePercent());
	const TArray<FLSSessionItem> ActiveEquipmentItems = BuildSignalActiveEquipmentItems(SaveSubsystem->GetChipEquipmentSlots(), InactiveSlotCount);
	OutCurrentNavigationProtocol = LSChipStats::AggregateChipProtocolTotals(ActiveEquipmentItems, this).Navigation;
	OutPreviousNavigationProtocol = LSChipStats::AggregateChipProtocolTotals(SaveSubsystem->GetChipEquipmentSlots(), this).Navigation;
}

bool ULSMinimapWidget::IsNavigationFeatureVisible(const FName EnableName, const int32 CurrentNavigationProtocol, const int32 PreviousNavigationProtocol, const bool bFallbackVisible) const
{
	UGameInstance* GameInstance = GetGameInstance();
	const ULSGameDataSubsystem* GameDataSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSGameDataSubsystem>() : nullptr;
	if (!GameDataSubsystem)
	{
		return bFallbackVisible;
	}

	const FLSProtocolUnlockRow* Row = GameDataSubsystem->FindProtocolUnlockRowByEnableName(ELSProtocolType::Navigation, EnableName, TEXT("MinimapProtocol"));
	return Row
		? GameDataSubsystem->IsProtocolUnlockVisible(*Row, CurrentNavigationProtocol, PreviousNavigationProtocol)
		: bFallbackVisible;
}

void ULSMinimapWidget::DrawPreviewData(
	const FGeometry& Geometry,
	FSlateWindowElementList& OutDrawElements,
	int32& LayerId,
	const FVector2D& Center,
	const float Radius,
	const int32 CurrentNavigationProtocol,
	const int32 PreviousNavigationProtocol) const
{
	const FLinearColor TerrainColor = FlattenTerrainColor(VisionTerrainColor);
	DrawFilledRectInCircle(OutDrawElements, ++LayerId, Geometry, Center + FVector2D(-Radius * 0.65f, -Radius * 0.48f), FVector2D(Radius * 0.68f, Radius * 0.28f), Center, Radius, TerrainColor);
	DrawFilledRectInCircle(OutDrawElements, ++LayerId, Geometry, Center + FVector2D( Radius * 0.08f, -Radius * 0.18f), FVector2D(Radius * 0.56f, Radius * 0.38f), Center, Radius, TerrainColor);
	DrawFilledCircle(OutDrawElements, ++LayerId, Geometry, Center + FVector2D(-Radius * 0.18f, Radius * 0.36f), Radius * 0.18f, TerrainColor);
	const TArray<FVector2D> PreviewPath = {
		Center + FVector2D(-Radius * 0.72f, Radius * 0.12f),
		Center + FVector2D(-Radius * 0.28f, Radius * 0.0f),
		Center + FVector2D( Radius * 0.12f, Radius * 0.22f),
		Center + FVector2D( Radius * 0.56f, Radius * 0.08f)
	};
	DrawPolyline(OutDrawElements, ++LayerId, Geometry, PreviewPath, TerrainColor, 2.0f, false);

	if (IsNavigationFeatureVisible(TEXT("Minimap_View_Angle"), CurrentNavigationProtocol, PreviousNavigationProtocol, true))
	{
		DrawSightCone(OutDrawElements, ++LayerId, Geometry, Center, FVector2D(0.0f, -1.0f), Radius, SightAngleDegrees, SightColor);
	}

	if (IsNavigationFeatureVisible(TEXT("Minimap_Looting_Object"), CurrentNavigationProtocol, PreviousNavigationProtocol, CurrentNavigationProtocol >= RevealPolicy.LootVisibleNavigation))
	{
		DrawFilledCircle(OutDrawElements, ++LayerId, Geometry, Center + FVector2D(-Radius * 0.42f, -Radius * 0.22f), 4.0f, FLinearColor(1.0f, 0.82f, 0.18f, 1.0f));
		DrawFilledCircle(OutDrawElements, ++LayerId, Geometry, Center + FVector2D( Radius * 0.34f,  Radius * 0.16f), 3.5f, FLinearColor(0.25f, 1.0f, 0.42f, 1.0f));
	}

	if (IsNavigationFeatureVisible(TEXT("Exit_Point"), CurrentNavigationProtocol, PreviousNavigationProtocol, CurrentNavigationProtocol >= RevealPolicy.ExtractionVisibleNavigation))
	{
		const FVector2D ExitPoint = Center + FVector2D(Radius * 0.62f, -Radius * 0.58f);
		const FLinearColor ExitColor(0.28f, 1.0f, 0.45f, 1.0f);
		DrawFilledCircle(OutDrawElements, ++LayerId, Geometry, ExitPoint, 5.0f, ExitColor);
		DrawText(OutDrawElements, ++LayerId, Geometry, ExitPoint + FVector2D(6.0f, -6.0f), FText::AsNumber(184), ExitColor);
	}

	if (IsNavigationFeatureVisible(TEXT("Minimap_Enemy"), CurrentNavigationProtocol, PreviousNavigationProtocol, CurrentNavigationProtocol >= RevealPolicy.EnemyVisibleNavigation))
	{
		DrawFilledCircle(OutDrawElements, ++LayerId, Geometry, Center + FVector2D(-Radius * 0.12f, -Radius * 0.52f), 4.0f, FLinearColor(1.0f, 0.12f, 0.1f, 1.0f));
	}

	if (IsNavigationFeatureVisible(TEXT("Player_Point"), CurrentNavigationProtocol, PreviousNavigationProtocol, true))
	{
		DrawFilledCircle(OutDrawElements, ++LayerId, Geometry, Center, 5.0f, PlayerColor);
	}
}

void ULSMinimapWidget::DrawShape(const FLSMinimapShapeSnapshot& Shape, const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements, const int32 LayerId, const FVector2D& Center, const float Radius, const float PixelsPerCm) const
{
	const FVector ShapeLocation = Shape.WorldTransform.GetLocation();
	const FVector2D ShapeCenter = ProjectWorldLocation(ShapeLocation, Center, PixelsPerCm);
	const FLinearColor TerrainColor = FlattenTerrainColor(Shape.FillColor);

	if (Shape.ShapeType == ELSMinimapShapeType::Box)
	{
		const FVector2D Size = Shape.Extent * 2.0f * PixelsPerCm;
		DrawFilledRectInCircle(OutDrawElements, LayerId, Geometry, ShapeCenter - (Size * 0.5f), Size, Center, Radius, TerrainColor);
		return;
	}

	if (Shape.ShapeType == ELSMinimapShapeType::Circle)
	{
		DrawFilledCircle(OutDrawElements, LayerId, Geometry, ShapeCenter, Shape.Extent.X * PixelsPerCm, TerrainColor);
		return;
	}

	TArray<FVector2D> Points;
	Points.Reserve(Shape.PolylinePoints.Num());
	for (const FVector& LocalPoint : Shape.PolylinePoints)
	{
		Points.Add(ProjectWorldLocation(Shape.WorldTransform.TransformPosition(LocalPoint), Center, PixelsPerCm));
	}

	if (Points.Num() > 1)
	{
		for (int32 PointIndex = 1; PointIndex < Points.Num(); ++PointIndex)
		{
			FVector2D Start = Points[PointIndex - 1];
			FVector2D End = Points[PointIndex];
			if (ClipSegmentToCircle(Start, End, Center, Radius))
			{
				FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(), { Start, End }, ESlateDrawEffect::None, TerrainColor, true, 3.0f);
			}
		}
	}
}

void ULSMinimapWidget::DrawMinimapObstacles(const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements, int32& LayerId, const FVector2D& Center, const float Radius, const float PixelsPerCm) const
{
	const UWorld* World = GetWorld();
	const ULSMinimapSubsystem* MinimapSubsystem = World ? World->GetSubsystem<ULSMinimapSubsystem>() : nullptr;
	if (!MinimapSubsystem)
	{
		return;
	}

	TArray<ULSMinimapObstacleComponent*> Obstacles;
	MinimapSubsystem->GetRegisteredObstacles(Obstacles);
	for (const ULSMinimapObstacleComponent* Obstacle : Obstacles)
	{
		if (!IsValid(Obstacle) || !Obstacle->IsMinimapVisible())
		{
			continue;
		}

		TArray<FBox> BoundsList;
		Obstacle->GatherObstacleBounds(BoundsList);
		for (const FBox& Bounds : BoundsList)
		{
			if (Bounds.IsValid)
			{
				DrawObstacleBounds(Bounds, Geometry, OutDrawElements, ++LayerId, Center, Radius, PixelsPerCm, FlattenTerrainColor(Obstacle->GetObstacleColor()), Obstacle->GetLineThickness());
			}
		}
	}
}

void ULSMinimapWidget::DrawObstacleBounds(const FBox& Bounds, const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements, const int32 LayerId, const FVector2D& Center, const float Radius, const float PixelsPerCm, const FLinearColor& Color, const float Thickness) const
{
	const TArray<FVector2D> Corners = {
		ProjectWorldLocation(FVector(Bounds.Min.X, Bounds.Min.Y, Bounds.Min.Z), Center, PixelsPerCm),
		ProjectWorldLocation(FVector(Bounds.Max.X, Bounds.Min.Y, Bounds.Min.Z), Center, PixelsPerCm),
		ProjectWorldLocation(FVector(Bounds.Max.X, Bounds.Max.Y, Bounds.Min.Z), Center, PixelsPerCm),
		ProjectWorldLocation(FVector(Bounds.Min.X, Bounds.Max.Y, Bounds.Min.Z), Center, PixelsPerCm)
	};

	for (int32 CornerIndex = 0; CornerIndex < Corners.Num(); ++CornerIndex)
	{
		FVector2D Start = Corners[CornerIndex];
		FVector2D End = Corners[(CornerIndex + 1) % Corners.Num()];
		if (ClipSegmentToCircle(Start, End, Center, Radius - 0.75f))
		{
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(), { Start, End }, ESlateDrawEffect::None, Color, true, Thickness);
		}
	}
}

void ULSMinimapWidget::DrawVisionTerrain(const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements, int32& LayerId, const FVector2D& Center, const float Radius, const float PixelsPerCm) const
{
	const UWorld* World = GetWorld();
	const ULSVisionSubsystem* VisionSubsystem = World ? World->GetSubsystem<ULSVisionSubsystem>() : nullptr;
	if (!VisionSubsystem)
	{
		return;
	}

	TSet<const AActor*> SurfaceOwners;
	for (const ULSVisionSurfaceComponent* Surface : VisionSubsystem->GetRegisteredVisionSurfaces())
	{
		if (!IsValid(Surface))
		{
			continue;
		}

		if (const AActor* Owner = Surface->GetOwner())
		{
			SurfaceOwners.Add(Owner);
		}

		FBox CombinedBounds(ForceInit);
		if (Surface->TargetPrimitives.Num() > 0)
		{
			for (const UPrimitiveComponent* Primitive : Surface->TargetPrimitives)
			{
				if (IsValid(Primitive))
				{
					CombinedBounds += Primitive->Bounds.GetBox();
				}
			}
		}
		else
		{
			TArray<UMeshComponent*> MeshComponents;
			if (AActor* Owner = Surface->GetOwner())
			{
				Owner->GetComponents<UMeshComponent>(MeshComponents);
			}
			for (const UMeshComponent* MeshComponent : MeshComponents)
			{
				if (IsValid(MeshComponent))
				{
					CombinedBounds += MeshComponent->Bounds.GetBox();
				}
			}
		}

		if (CombinedBounds.IsValid)
		{
			DrawVisionSurfaceBounds(CombinedBounds, Geometry, OutDrawElements, ++LayerId, Center, Radius, PixelsPerCm);
		}
	}

	for (const ULSVisionOccluderComponent* Occluder : VisionSubsystem->GetRegisteredOccluders())
	{
		if (!IsValid(Occluder) || SurfaceOwners.Contains(Occluder->GetOwner()))
		{
			continue;
		}

		DrawVisionOccluderSegments(Occluder->GetSegments(), Geometry, OutDrawElements, ++LayerId, Center, Radius, PixelsPerCm);
	}
}

void ULSMinimapWidget::DrawVisionSurfaceBounds(const FBox& Bounds, const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements, const int32 LayerId, const FVector2D& Center, const float Radius, const float PixelsPerCm) const
{
	const TArray<FVector2D> Corners = {
		ProjectWorldLocation(FVector(Bounds.Min.X, Bounds.Min.Y, Bounds.Min.Z), Center, PixelsPerCm),
		ProjectWorldLocation(FVector(Bounds.Max.X, Bounds.Min.Y, Bounds.Min.Z), Center, PixelsPerCm),
		ProjectWorldLocation(FVector(Bounds.Max.X, Bounds.Max.Y, Bounds.Min.Z), Center, PixelsPerCm),
		ProjectWorldLocation(FVector(Bounds.Min.X, Bounds.Max.Y, Bounds.Min.Z), Center, PixelsPerCm)
	};
	const FLinearColor TerrainColor = FlattenTerrainColor(VisionTerrainColor);
	DrawFilledPolygonInCircle(OutDrawElements, LayerId, Geometry, Corners, Center, Radius, TerrainColor);
	for (int32 CornerIndex = 0; CornerIndex < Corners.Num(); ++CornerIndex)
	{
		FVector2D Start = Corners[CornerIndex];
		FVector2D End = Corners[(CornerIndex + 1) % Corners.Num()];
		if (ClipSegmentToCircle(Start, End, Center, Radius - 0.75f))
		{
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(), { Start, End }, ESlateDrawEffect::None, TerrainColor, true, 1.5f);
		}
	}
}

void ULSMinimapWidget::DrawVisionOccluderSegments(const TArray<FLSVisionSegment2D>& Segments, const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements, const int32 LayerId, const FVector2D& Center, const float Radius, const float PixelsPerCm) const
{
	for (const FLSVisionSegment2D& Segment : Segments)
	{
		const FVector Start(Segment.Start.X, Segment.Start.Y, 0.0f);
		const FVector End(Segment.End.X, Segment.End.Y, 0.0f);
		FVector2D ProjectedStart = ProjectWorldLocation(Start, Center, PixelsPerCm);
		FVector2D ProjectedEnd = ProjectWorldLocation(End, Center, PixelsPerCm);
		if (ClipSegmentToCircle(ProjectedStart, ProjectedEnd, Center, Radius))
		{
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(), { ProjectedStart, ProjectedEnd }, ESlateDrawEffect::None, FlattenTerrainColor(VisionTerrainColor), true, 3.0f);
		}
	}

}

void ULSMinimapWidget::DrawSightCone(FSlateWindowElementList& OutDrawElements, const int32 LayerId, const FGeometry& Geometry, const FVector2D& Center, const FVector2D& Forward, const float Radius, const float AngleDegrees, const FLinearColor& Color) const
{
	const FVector2D ForwardNormal = Forward.GetSafeNormal();
	if (ForwardNormal.IsNearlyZero() || Radius <= 1.0f)
	{
		return;
	}

	const float MinDot = FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(AngleDegrees, 1.0f, 180.0f) * 0.5f));
	const float RadiusSquared = FMath::Square(Radius);
	const FSlateBrush* Brush = FCoreStyle::Get().GetBrush("WhiteBrush");
	for (float LocalY = -Radius; LocalY < Radius; LocalY += CircleFillStep)
	{
		bool bHasRun = false;
		float RunStartX = 0.0f;
		float RunEndX = 0.0f;
		const float RowHeight = FMath::Min(CircleFillStep, Radius - LocalY);
		const float SampleY = LocalY + RowHeight * 0.5f;

		for (float LocalX = -Radius; LocalX <= Radius; LocalX += CircleFillStep)
		{
			const FVector2D Offset(LocalX, SampleY);
			const float DistanceSquared = Offset.SizeSquared();
			const bool bInsideCone = DistanceSquared <= RadiusSquared && FVector2D::DotProduct(Offset.GetSafeNormal(), ForwardNormal) >= MinDot;
			if (bInsideCone)
			{
				if (!bHasRun)
				{
					RunStartX = LocalX;
					bHasRun = true;
				}
				RunEndX = LocalX + CircleFillStep;
				continue;
			}

			if (bHasRun)
			{
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					LayerId,
					Geometry.ToPaintGeometry(FVector2f(FVector2D(RunEndX - RunStartX, RowHeight)), FSlateLayoutTransform(FVector2f(Center + FVector2D(RunStartX, LocalY)))),
					Brush,
					ESlateDrawEffect::None,
					Color);
				bHasRun = false;
			}
		}

		if (bHasRun)
		{
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId,
				Geometry.ToPaintGeometry(FVector2f(FVector2D(RunEndX - RunStartX, RowHeight)), FSlateLayoutTransform(FVector2f(Center + FVector2D(RunStartX, LocalY)))),
				Brush,
				ESlateDrawEffect::None,
				Color);
		}
	}

	const float HalfAngleRad = FMath::DegreesToRadians(FMath::Clamp(AngleDegrees, 1.0f, 180.0f) * 0.5f);
	const FVector2D Right(ForwardNormal.Y, -ForwardNormal.X);
	const FVector2D LeftDirection = (ForwardNormal * FMath::Cos(HalfAngleRad)) - (Right * FMath::Sin(HalfAngleRad));
	const FVector2D RightDirection = (ForwardNormal * FMath::Cos(HalfAngleRad)) + (Right * FMath::Sin(HalfAngleRad));
	TArray<FVector2D> OutlinePoints;
	OutlinePoints.Reserve(35);
	OutlinePoints.Add(Center);
	OutlinePoints.Add(Center + LeftDirection * (Radius - 0.75f));
	for (int32 ArcIndex = 1; ArcIndex < 32; ++ArcIndex)
	{
		const float Alpha = static_cast<float>(ArcIndex) / 32.0f;
		const float Angle = FMath::Lerp(-HalfAngleRad, HalfAngleRad, Alpha);
		const FVector2D ArcDirection = (ForwardNormal * FMath::Cos(Angle)) + (Right * FMath::Sin(Angle));
		OutlinePoints.Add(Center + ArcDirection * (Radius - 0.75f));
	}
	OutlinePoints.Add(Center + RightDirection * (Radius - 0.75f));
	OutlinePoints.Add(Center);
	DrawPolyline(OutDrawElements, LayerId, Geometry, OutlinePoints, Color, 1.5f, false);
}

void ULSMinimapWidget::DrawFilledPolygonInCircle(FSlateWindowElementList& OutDrawElements, const int32 LayerId, const FGeometry& Geometry, const TArray<FVector2D>& Points, const FVector2D& Center, const float Radius, const FLinearColor& Color) const
{
	if (Points.Num() < 3)
	{
		return;
	}

	float MinY = Points[0].Y;
	float MaxY = Points[0].Y;
	for (const FVector2D& Point : Points)
	{
		MinY = FMath::Min(MinY, Point.Y);
		MaxY = FMath::Max(MaxY, Point.Y);
	}

	MinY = FMath::Max(MinY, Center.Y - Radius);
	MaxY = FMath::Min(MaxY, Center.Y + Radius);
	if (MinY >= MaxY)
	{
		return;
	}

	const FSlateBrush* Brush = FCoreStyle::Get().GetBrush("WhiteBrush");
	TArray<float> Intersections;
	Intersections.Reserve(Points.Num());

	for (float Y = MinY; Y < MaxY; Y += CircleFillStep)
	{
		const float RowHeight = FMath::Min(CircleFillStep, MaxY - Y);
		const float SampleY = Y + RowHeight * 0.5f;
		Intersections.Reset();

		for (int32 PointIndex = 0; PointIndex < Points.Num(); ++PointIndex)
		{
			const FVector2D& A = Points[PointIndex];
			const FVector2D& B = Points[(PointIndex + 1) % Points.Num()];
			if ((A.Y <= SampleY && B.Y > SampleY) || (B.Y <= SampleY && A.Y > SampleY))
			{
				const float Alpha = (SampleY - A.Y) / (B.Y - A.Y);
				Intersections.Add(FMath::Lerp(A.X, B.X, Alpha));
			}
		}

		if (Intersections.Num() < 2)
		{
			continue;
		}

		Intersections.Sort();
		const float CircleHalfWidth = FMath::Sqrt(FMath::Max(0.0f, Radius * Radius - FMath::Square(SampleY - Center.Y)));
		const float CircleMinX = Center.X - CircleHalfWidth;
		const float CircleMaxX = Center.X + CircleHalfWidth;

		for (int32 IntersectionIndex = 1; IntersectionIndex < Intersections.Num(); IntersectionIndex += 2)
		{
			const float MinX = FMath::Max(Intersections[IntersectionIndex - 1], CircleMinX);
			const float MaxX = FMath::Min(Intersections[IntersectionIndex], CircleMaxX);
			if (MinX >= MaxX)
			{
				continue;
			}

			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId,
				Geometry.ToPaintGeometry(FVector2f(FVector2D(MaxX - MinX, RowHeight)), FSlateLayoutTransform(FVector2f(MinX, Y))),
				Brush,
				ESlateDrawEffect::None,
				Color);
		}
	}
}

void ULSMinimapWidget::DrawFilledRectInCircle(FSlateWindowElementList& OutDrawElements, const int32 LayerId, const FGeometry& Geometry, const FVector2D& TopLeft, const FVector2D& Size, const FVector2D& Center, const float Radius, const FLinearColor& Color) const
{
	const FVector2D BottomRight = TopLeft + Size;
	const float MinY = FMath::Max(TopLeft.Y, Center.Y - Radius);
	const float MaxY = FMath::Min(BottomRight.Y, Center.Y + Radius);
	if (MinY >= MaxY)
	{
		return;
	}

	const FSlateBrush* Brush = FCoreStyle::Get().GetBrush("WhiteBrush");
	const float Step = CircleFillStep;
	for (float Y = MinY; Y < MaxY; Y += Step)
	{
		const float RowHeight = FMath::Min(Step, MaxY - Y);
		const float RowCenterY = Y + RowHeight * 0.5f;
		const float HalfWidth = FMath::Sqrt(FMath::Max(0.0f, Radius * Radius - FMath::Square(RowCenterY - Center.Y)));
		const float MinX = FMath::Max(TopLeft.X, Center.X - HalfWidth);
		const float MaxX = FMath::Min(BottomRight.X, Center.X + HalfWidth);
		if (MinX >= MaxX)
		{
			continue;
		}

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			Geometry.ToPaintGeometry(FVector2f(FVector2D(MaxX - MinX, RowHeight)), FSlateLayoutTransform(FVector2f(MinX, Y))),
			Brush,
			ESlateDrawEffect::None,
			Color);
	}
}

bool ULSMinimapWidget::ClipSegmentToCircle(FVector2D& Start, FVector2D& End, const FVector2D& Center, const float Radius) const
{
	const bool bStartInside = FVector2D::DistSquared(Start, Center) <= FMath::Square(Radius);
	const bool bEndInside = FVector2D::DistSquared(End, Center) <= FMath::Square(Radius);
	if (bStartInside && bEndInside)
	{
		return true;
	}

	const FVector2D Direction = End - Start;
	const FVector2D FromCenter = Start - Center;
	const float A = FVector2D::DotProduct(Direction, Direction);
	if (A <= UE_KINDA_SMALL_NUMBER)
	{
		return bStartInside;
	}

	const float B = 2.0f * FVector2D::DotProduct(FromCenter, Direction);
	const float C = FVector2D::DotProduct(FromCenter, FromCenter) - FMath::Square(Radius);
	const float Discriminant = FMath::Square(B) - (4.0f * A * C);
	if (Discriminant < 0.0f)
	{
		return false;
	}

	const float SqrtDiscriminant = FMath::Sqrt(Discriminant);
	const float T0 = (-B - SqrtDiscriminant) / (2.0f * A);
	const float T1 = (-B + SqrtDiscriminant) / (2.0f * A);
	const float MinT = FMath::Max(0.0f, FMath::Min(T0, T1));
	const float MaxT = FMath::Min(1.0f, FMath::Max(T0, T1));
	if (MinT > MaxT)
	{
		return false;
	}

	const FVector2D OriginalStart = Start;
	Start = OriginalStart + Direction * MinT;
	End = OriginalStart + Direction * MaxT;
	return true;
}

FLinearColor ULSMinimapWidget::FlattenTerrainColor(const FLinearColor Color) const
{
	const float Alpha = FMath::Clamp(Color.A, 0.0f, 1.0f);
	const float BackgroundAlpha = FMath::Clamp(BackgroundColor.A, 0.0f, 1.0f);
	const FLinearColor BaseColor(
		BackgroundColor.R * BackgroundAlpha,
		BackgroundColor.G * BackgroundAlpha,
		BackgroundColor.B * BackgroundAlpha,
		1.0f);

	return FLinearColor(
		(Color.R * Alpha) + (BaseColor.R * (1.0f - Alpha)),
		(Color.G * Alpha) + (BaseColor.G * (1.0f - Alpha)),
		(Color.B * Alpha) + (BaseColor.B * (1.0f - Alpha)),
		1.0f);
}

void ULSMinimapWidget::DrawFilledCircle(FSlateWindowElementList& OutDrawElements, const int32 LayerId, const FGeometry& Geometry, const FVector2D& Center, const float Radius, const FLinearColor& Color) const
{
	const FSlateBrush* Brush = FCoreStyle::Get().GetBrush("WhiteBrush");
	const float Step = CircleFillStep;
	for (float Y = -Radius; Y < Radius; Y += Step)
	{
		const float RowHeight = FMath::Min(Step, Radius - Y);
		const float RowCenterY = Y + RowHeight * 0.5f;
		const float HalfWidth = FMath::Sqrt(FMath::Max(0.0f, Radius * Radius - RowCenterY * RowCenterY));
		const FVector2D LineSize(HalfWidth * 2.0f, RowHeight);
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			Geometry.ToPaintGeometry(FVector2f(LineSize), FSlateLayoutTransform(FVector2f(Center + FVector2D(-HalfWidth, Y)))),
			Brush,
			ESlateDrawEffect::None,
			Color);
	}
}

void ULSMinimapWidget::DrawCircleOutline(FSlateWindowElementList& OutDrawElements, const int32 LayerId, const FGeometry& Geometry, const FVector2D& Center, const float Radius, const FLinearColor& Color, const float Thickness) const
{
	TArray<FVector2D> Points;
	Points.Reserve(97);
	for (int32 PointIndex = 0; PointIndex <= 96; ++PointIndex)
	{
		const float Angle = (static_cast<float>(PointIndex) / 96.0f) * (2.0f * UE_PI);
		Points.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
	}

	FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, Color, true, Thickness);
}

void ULSMinimapWidget::DrawPolyline(FSlateWindowElementList& OutDrawElements, const int32 LayerId, const FGeometry& Geometry, const TArray<FVector2D>& Points, const FLinearColor& Color, const float Thickness, const bool bClosed) const
{
	if (Points.Num() < 2)
	{
		return;
	}

	TArray<FVector2D> DrawPoints = Points;
	if (bClosed)
	{
		DrawPoints.Add(Points[0]);
	}

	FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(), DrawPoints, ESlateDrawEffect::None, Color, true, Thickness);
}

void ULSMinimapWidget::DrawText(FSlateWindowElementList& OutDrawElements, const int32 LayerId, const FGeometry& Geometry, const FVector2D& Position, const FText& Text, const FLinearColor& Color) const
{
	const FSlateFontInfo FontInfo = FCoreStyle::GetDefaultFontStyle("Regular", 10);
	FSlateDrawElement::MakeText(
		OutDrawElements,
		LayerId,
		Geometry.ToPaintGeometry(FVector2f(40.0f, 14.0f), FSlateLayoutTransform(FVector2f(Position))),
		Text,
		FontInfo,
		ESlateDrawEffect::None,
		Color);
}
