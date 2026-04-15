#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/SceneComponent.h"
#include "Vision/LSVisionTypes.h"
#include "LSVisionOccluderComponent.generated.h"

class UBoxComponent;
class UPrimitiveComponent;
class USceneComponent;

UENUM(BlueprintType)
enum class ELSVisionOccluderSourceMode : uint8
{
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

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision|Source")
	ELSVisionOccluderSourceMode SourceMode = ELSVisionOccluderSourceMode::PrimitiveBounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision|Source")
	bool bAutoFindOwnerComponents = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision|Source")
	bool bRebuildOnTransformChanged = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision|Source", meta = (EditCondition = "SourceMode == ELSVisionOccluderSourceMode::BoxComponent", EditConditionHides))
	TObjectPtr<UBoxComponent> SourceBoxComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision|Source", meta = (EditCondition = "SourceMode == ELSVisionOccluderSourceMode::PrimitiveBounds", EditConditionHides))
	TObjectPtr<UPrimitiveComponent> SourcePrimitiveComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision|Manual", meta = (EditCondition = "SourceMode == ELSVisionOccluderSourceMode::ManualSegments", EditConditionHides))
	TArray<FLSVisionSegment2D> ManualSegments;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vision")
	TArray<FLSVisionSegment2D> Segments;

	UFUNCTION(BlueprintCallable, Category = "Vision")
	void RebuildSegments();

	const TArray<FLSVisionSegment2D>& GetSegments() const
	{
		return Segments;
	}

private:
	void UpdateObservedComponentBinding();
	void HandleObservedComponentTransformUpdated(USceneComponent* UpdatedComponent, EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport);
	void BuildSegmentsFromBox(const UBoxComponent* BoxComponent, TArray<FLSVisionSegment2D>& OutSegments) const;
	void BuildSegmentsFromPrimitiveBounds(const UPrimitiveComponent* PrimitiveComponent, TArray<FLSVisionSegment2D>& OutSegments) const;
	void AddRectangleSegments(const FVector2D& Min, const FVector2D& Max, TArray<FLSVisionSegment2D>& OutSegments) const;

	UBoxComponent* ResolveBoxComponent() const;
	UPrimitiveComponent* ResolvePrimitiveComponent() const;
	USceneComponent* ResolveObservedSceneComponent() const;

	TWeakObjectPtr<USceneComponent> ObservedSceneComponent;
};
