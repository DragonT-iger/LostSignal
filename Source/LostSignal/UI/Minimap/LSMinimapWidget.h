#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Minimap/LSMinimapTypes.h"
#include "LSMinimapWidget.generated.h"

class ULSMinimapMarkerComponent;
class ULSMinimapObstacleComponent;
struct FLSVisionSegment2D;

UCLASS()
class LOSTSIGNAL_API ULSMinimapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="LS/UI|Minimap")
	void InitializeMinimapForPawn(APawn* InPawn);

	UFUNCTION(BlueprintCallable, Category="LS/UI|Minimap")
	void SetPreviewNavigationLevels(int32 CurrentNavigationProtocol, int32 PreviousNavigationProtocol);

	UFUNCTION(BlueprintCallable, Category="LS/UI|Minimap")
	void ClearPreviewNavigationLevels();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

private:
	FVector2D ProjectWorldLocation(FVector WorldLocation, const FVector2D& Center, float PixelsPerCm) const;
	FVector2D ProjectWorldDirection(FVector WorldDirection) const;
	FVector2D ClampToMinimapEdge(const FVector2D& Point, const FVector2D& Center, float Radius) const;
	bool ResolveMinimapViewAxes(FVector& OutViewUp, FVector& OutViewRight) const;
	bool ShouldDrawMarker(const FLSMinimapMarkerSnapshot& Marker, const FVector2D& ProjectedPoint, const FVector2D& Center, float Radius, int32 CurrentNavigationProtocol, int32 PreviousNavigationProtocol) const;
	bool IsMarkerInSight(const FLSMinimapMarkerSnapshot& Marker) const;
	void ResolveNavigationProtocolLevels(int32& OutCurrentNavigationProtocol, int32& OutPreviousNavigationProtocol) const;
	bool IsNavigationFeatureVisible(FName EnableName, int32 CurrentNavigationProtocol, int32 PreviousNavigationProtocol, bool bFallbackVisible) const;
	bool ShouldDrawMarkerDistance(const FLSMinimapMarkerSnapshot& Marker) const;
	FText BuildMarkerDistanceText(const FLSMinimapMarkerSnapshot& Marker) const;
	void DrawPreviewData(const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements, int32& LayerId, const FVector2D& Center, float Radius, int32 CurrentNavigationProtocol, int32 PreviousNavigationProtocol) const;
	void DrawShape(const FLSMinimapShapeSnapshot& Shape, const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FVector2D& Center, float Radius, float PixelsPerCm) const;
	void DrawMinimapObstacles(const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements, int32& LayerId, const FVector2D& Center, float Radius, float PixelsPerCm) const;
	void DrawObstacleBounds(const FBox& Bounds, const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FVector2D& Center, float Radius, float PixelsPerCm, const FLinearColor& Color, float Thickness) const;
	void DrawVisionTerrain(const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements, int32& LayerId, const FVector2D& Center, float Radius, float PixelsPerCm) const;
	void DrawVisionSurfaceBounds(const FBox& Bounds, const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FVector2D& Center, float Radius, float PixelsPerCm) const;
	void DrawVisionOccluderSegments(const TArray<FLSVisionSegment2D>& Segments, const FGeometry& Geometry, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FVector2D& Center, float Radius, float PixelsPerCm) const;
	void DrawSightCone(FSlateWindowElementList& OutDrawElements, int32 LayerId, const FGeometry& Geometry, const FVector2D& Center, const FVector2D& Forward, float Radius, float AngleDegrees, const FLinearColor& Color) const;
	void DrawFilledPolygonInCircle(FSlateWindowElementList& OutDrawElements, int32 LayerId, const FGeometry& Geometry, const TArray<FVector2D>& Points, const FVector2D& Center, float Radius, const FLinearColor& Color) const;
	void DrawFilledRectInCircle(FSlateWindowElementList& OutDrawElements, int32 LayerId, const FGeometry& Geometry, const FVector2D& TopLeft, const FVector2D& Size, const FVector2D& Center, float Radius, const FLinearColor& Color) const;
	void DrawFilledCircle(FSlateWindowElementList& OutDrawElements, int32 LayerId, const FGeometry& Geometry, const FVector2D& Center, float Radius, const FLinearColor& Color) const;
	void DrawCircleOutline(FSlateWindowElementList& OutDrawElements, int32 LayerId, const FGeometry& Geometry, const FVector2D& Center, float Radius, const FLinearColor& Color, float Thickness) const;
	void DrawPolyline(FSlateWindowElementList& OutDrawElements, int32 LayerId, const FGeometry& Geometry, const TArray<FVector2D>& Points, const FLinearColor& Color, float Thickness, bool bClosed) const;
	void DrawText(FSlateWindowElementList& OutDrawElements, int32 LayerId, const FGeometry& Geometry, const FVector2D& Position, const FText& Text, const FLinearColor& Color) const;
	bool ClipSegmentToCircle(FVector2D& Start, FVector2D& End, const FVector2D& Center, float Radius) const;
	FLinearColor FlattenTerrainColor(FLinearColor Color) const;

	UPROPERTY(Transient)
	TWeakObjectPtr<APawn> ObservedPawn;

	UPROPERTY(Transient)
	bool bUsePreviewNavigationLevels = false;

	UPROPERTY(Transient)
	int32 PreviewCurrentNavigationProtocol = 0;

	UPROPERTY(Transient)
	int32 PreviewPreviousNavigationProtocol = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Minimap", meta=(AllowPrivateAccess="true", ClampMin="100.0"))
	float ViewRadiusCm = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Minimap", meta=(AllowPrivateAccess="true", ClampMin="1.0", ClampMax="180.0"))
	float SightAngleDegrees = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Minimap", meta=(AllowPrivateAccess="true", ClampMin="100.0"))
	float SightRadiusCm = 1600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Minimap", meta=(AllowPrivateAccess="true"))
	FLSMinimapRevealPolicy RevealPolicy;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Minimap", meta=(AllowPrivateAccess="true"))
	FLinearColor BackgroundColor = FLinearColor(0.02f, 0.025f, 0.03f, 0.78f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Minimap", meta=(AllowPrivateAccess="true"))
	FLinearColor SightColor = FLinearColor(0.25f, 0.75f, 1.0f, 0.3f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Minimap", meta=(AllowPrivateAccess="true"))
	FLinearColor PlayerColor = FLinearColor(0.1f, 0.8f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Minimap", meta=(AllowPrivateAccess="true"))
	FLinearColor VisionTerrainColor = FLinearColor(0.42f, 0.1f, 0.85f, 0.45f);
};
