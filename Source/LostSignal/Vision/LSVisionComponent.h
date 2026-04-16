#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Vision/LSVisionTypes.h"
#include "LSVisionComponent.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;

UCLASS(ClassGroup = (Vision), meta = (BlueprintSpawnableComponent))
class LOSTSIGNAL_API ULSVisionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSVisionComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void UpdateVisionPolygon();
	bool IsLocalVisionController() const;
	bool IsPointVisibleInCurrentVision(const FVector2D& Point2D) const;
	void UpdateVisionTargets(const FVector2D& VisionOrigin2D);
	void DrawDebugVisionRays() const;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision")
	float VisionRadius = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision")
	float HalfFOVDegrees = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision")
	float MaxRayDistance = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision", meta = (ClampMin = "0.01"))
	float UpdateInterval = 0.016f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision", meta = (ClampMin = "0.01"))
	float DivideAngleDegree = 0.5f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vision")
	FLSVisionPolygonData CurrentPolygon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision|Debug")
	bool bDrawDebugRays = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision|Debug")
	FColor DebugRayColor = FColor::Yellow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision|Debug", meta = (ClampMin = "0.0"))
	float DebugRayDuration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision|Debug", meta = (ClampMin = "0.0"))
	float DebugRayThickness = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision|Debug")
	float DebugRayZOffset = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vision")
	TObjectPtr<UMaterialInterface> PostProcessMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision")
	FName VisibilityMaskTextureParamName = TEXT("VisibilityMaskRT");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision")
	FName MaskOriginParamName = TEXT("MaskOriginWS");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision")
	FName MaskExtentParamName = TEXT("MaskExtent");

private:
	FTimerHandle VisionUpdateTimerHandle;
	TObjectPtr<UMaterialInstanceDynamic> PostProcessMID;
};
