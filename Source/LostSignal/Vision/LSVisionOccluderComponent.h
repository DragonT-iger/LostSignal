#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/SceneComponent.h"
#include "Vision/LSVisionTypes.h"
#include "LSVisionOccluderComponent.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UPrimitiveComponent;
class USceneComponent;

UENUM(BlueprintType)
enum class ELSVisionOccluderSourceMode : uint8
{
	CollisionGeometry UMETA(DisplayName = "Collision Geometry"),
	MeshBounds UMETA(DisplayName = "Mesh Bounds"),
	PrimitiveBounds UMETA(DisplayName = "Primitive Bounds"),
	BoxComponent UMETA(DisplayName = "Box Component"),
	ManualSegments UMETA(DisplayName = "Manual Segments")
};

UCLASS(ClassGroup = (Vision), meta = (BlueprintSpawnableComponent))
class LOSTSIGNAL_API ULSVisionOccluderComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSVisionOccluderComponent();

protected:
	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnUnregister() override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|Source")
	ELSVisionOccluderSourceMode SourceMode = ELSVisionOccluderSourceMode::PrimitiveBounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|Source")
	bool bAutoFindOwnerComponents = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|Source")
	bool bRebuildOnTransformChanged = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|Source", meta = (EditCondition = "SourceMode == ELSVisionOccluderSourceMode::BoxComponent", EditConditionHides))
	TObjectPtr<UBoxComponent> SourceBoxComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|Source", meta = (EditCondition = "SourceMode == ELSVisionOccluderSourceMode::CollisionGeometry || SourceMode == ELSVisionOccluderSourceMode::PrimitiveBounds || SourceMode == ELSVisionOccluderSourceMode::MeshBounds", EditConditionHides))
	TObjectPtr<UPrimitiveComponent> SourcePrimitiveComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|Manual", meta = (EditCondition = "SourceMode == ELSVisionOccluderSourceMode::ManualSegments", EditConditionHides))
	TArray<FLSVisionSegment2D> ManualSegments;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LS/Vision")
	TArray<FLSVisionSegment2D> Segments;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|Debug")
	bool bDrawDebugSegments = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|Debug")
	FColor DebugSegmentColor = FColor::Green;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|Debug", meta = (ClampMin = "0.0"))
	float DebugDrawZOffset = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|Debug", meta = (ClampMin = "0.0"))
	float DebugDrawThickness = 2.0f;

	UFUNCTION(BlueprintCallable, Category = "LS/Vision")
	void RebuildSegments();

	const TArray<FLSVisionSegment2D>& GetSegments() const
	{
		return Segments;
	}

private:
	void BuildSegmentsForSourceMode(float SliceZ, TArray<FLSVisionSegment2D>& OutSegments) const;
	void DrawDebugSegments() const;
	void UpdateObservedComponentBinding();
	void HandleObservedComponentTransformUpdated(USceneComponent* UpdatedComponent, EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport);
	// 콜리전 형상을 수평면(Z=SliceZ)으로 잘라 단면 윤곽점을 모은 뒤 세그먼트로 만든다(눈높이가 아닌 바닥 평면 기준).
	float ResolveSliceZ() const;

	UBoxComponent* ResolveBoxComponent() const;
	UPrimitiveComponent* ResolveMeshPrimitiveComponent() const;
	UPrimitiveComponent* ResolvePrimitiveComponent() const;
	USceneComponent* ResolveObservedSceneComponent() const;

	TWeakObjectPtr<USceneComponent> ObservedSceneComponent;
};
