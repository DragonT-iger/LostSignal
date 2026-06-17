#pragma once

#include "CoreMinimal.h"
#include "LSMinimapTypes.generated.h"

class UTexture2D;

UENUM(BlueprintType)
enum class ELSMinimapMarkerType : uint8
{
	Player,
	Enemy,
	Loot,
	DroppedItem,
	Extraction
};

UENUM(BlueprintType)
enum class ELSMinimapShapeType : uint8
{
	Box,
	Circle,
	Polyline
};

USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSMinimapRevealPolicy
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Minimap", meta=(ClampMin="0"))
	int32 EnemyVisibleNavigation = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Minimap", meta=(ClampMin="0"))
	int32 EnemyAlwaysVisibleNavigation = 6;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Minimap", meta=(ClampMin="0"))
	int32 LootVisibleNavigation = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Minimap", meta=(ClampMin="0"))
	int32 ExtractionVisibleNavigation = 1;
};

USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSMinimapMarkerSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="LS/Minimap")
	TObjectPtr<AActor> OwnerActor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="LS/Minimap")
	ELSMinimapMarkerType MarkerType = ELSMinimapMarkerType::Loot;

	UPROPERTY(BlueprintReadOnly, Category="LS/Minimap")
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category="LS/Minimap")
	FLinearColor Color = FLinearColor::White;

	UPROPERTY(BlueprintReadOnly, Category="LS/Minimap")
	float DrawRadius = 4.0f;

	UPROPERTY(BlueprintReadOnly, Category="LS/Minimap")
	TObjectPtr<UTexture2D> MarkerTexture = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="LS/Minimap")
	FVector2D TextureDrawSize = FVector2D(16.0f, 16.0f);

	UPROPERTY(BlueprintReadOnly, Category="LS/Minimap")
	int32 Priority = 0;

	UPROPERTY(BlueprintReadOnly, Category="LS/Minimap")
	bool bAlwaysVisible = false;

	UPROPERTY(BlueprintReadOnly, Category="LS/Minimap")
	bool bVisible = true;
};

USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSMinimapShapeSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="LS/Minimap")
	ELSMinimapShapeType ShapeType = ELSMinimapShapeType::Box;

	UPROPERTY(BlueprintReadOnly, Category="LS/Minimap")
	FTransform WorldTransform = FTransform::Identity;

	UPROPERTY(BlueprintReadOnly, Category="LS/Minimap")
	FVector2D Extent = FVector2D(100.0f, 100.0f);

	UPROPERTY(BlueprintReadOnly, Category="LS/Minimap")
	TArray<FVector> PolylinePoints;

	UPROPERTY(BlueprintReadOnly, Category="LS/Minimap")
	FLinearColor FillColor = FLinearColor(0.42f, 0.1f, 0.85f, 0.45f);
};
