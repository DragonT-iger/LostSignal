#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LSVisionSurfaceComponent.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;
class UMeshComponent;
class UPrimitiveComponent;
class UTextureRenderTarget2D;

UCLASS(ClassGroup = (Vision), meta = (BlueprintSpawnableComponent))
class LOSTSIGNAL_API ULSVisionSurfaceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSVisionSurfaceComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision")
	TArray<TObjectPtr<UPrimitiveComponent>> TargetPrimitives;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision")
	bool bUseExistingMaterials = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision")
	TObjectPtr<UMaterialInterface> VisionMaterialOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision")
	FName VisibilityMaskTextureParamName = TEXT("VisibilityMaskRT");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision")
	FName MaskOriginParamName = TEXT("MaskOriginWS");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision")
	FName MaskExtentParamName = TEXT("MaskExtent");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision")
	FName Forward2DParamName = TEXT("PlayerForward2D");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision")
	FName SurfacePushParamName = TEXT("MaskSurfacePush");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LS/Vision")
	TArray<TObjectPtr<UMaterialInstanceDynamic>> VisionMaterialInstances;

	UFUNCTION(BlueprintCallable, Category = "LS/Vision")
	void InitializeVisionMaterials();

	UFUNCTION(BlueprintCallable, Category = "LS/Vision")
	void ApplyVisionParameters(UTextureRenderTarget2D* VisibilityMaskRT, const FVector& MaskOriginWS, float MaskExtent, const FVector2D& PlayerForward2D);

private:
	void GatherTargetMeshComponents(TArray<UMeshComponent*>& OutMeshComponents) const;
};
