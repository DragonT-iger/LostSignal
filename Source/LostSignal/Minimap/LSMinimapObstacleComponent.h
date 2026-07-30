#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Vision/LSVisionTypes.h"
#include "LSMinimapObstacleComponent.generated.h"

class UPrimitiveComponent;
class USceneComponent;

UCLASS(ClassGroup=(LS), meta=(BlueprintSpawnableComponent))
class LOSTSIGNAL_API ULSMinimapObstacleComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSMinimapObstacleComponent();

	void AddTargetPrimitive(UPrimitiveComponent* Primitive);
	void GatherObstacleSegments(TArray<FLSVisionSegment2D>& OutSegments);

	FLinearColor GetObstacleColor() const { return ObstacleColor; }
	float GetLineThickness() const { return LineThickness; }
	bool IsMinimapVisible() const { return bVisibleOnMinimap; }

protected:
	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnUnregister() override;

private:
	void RebuildSegments();
	void GatherSourcePrimitives(TArray<UPrimitiveComponent*>& OutPrimitives) const;
	void ClearObservedComponentBindings();
	void UpdateObservedComponentBindings();
	void HandleObservedComponentTransformUpdated(USceneComponent* UpdatedComponent, EUpdateTransformFlags UpdateTransformFlags, ETeleportType Teleport);
	float ResolveSliceZ() const;
	bool ShouldUsePrimitive(const UPrimitiveComponent* Primitive) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Minimap", meta=(AllowPrivateAccess="true"))
	TArray<TObjectPtr<UPrimitiveComponent>> TargetPrimitives;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Minimap", meta=(AllowPrivateAccess="true"))
	bool bUseOwnerBlockingPrimitives = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Minimap", meta=(AllowPrivateAccess="true"))
	FLinearColor ObstacleColor = FLinearColor(0.42f, 0.1f, 0.85f, 0.85f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Minimap", meta=(AllowPrivateAccess="true", ClampMin="0.5"))
	float LineThickness = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Minimap", meta=(AllowPrivateAccess="true"))
	bool bVisibleOnMinimap = true;

	TArray<FLSVisionSegment2D> CachedSegments;
	TArray<TWeakObjectPtr<USceneComponent>> ObservedSceneComponents;
	float CachedSliceZ = TNumericLimits<float>::Lowest();
};
