#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "LSVisionSettings.generated.h"

class ALSVisionMaskRenderer;
class UMaterialInterface;
class UTextureRenderTarget2D;

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "LS Vision"))
class LOSTSIGNAL_API ULSVisionSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Vision")
	TSoftClassPtr<ALSVisionMaskRenderer> MaskRendererClass;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Vision")
	TSoftObjectPtr<UTextureRenderTarget2D> VisibilityMaskRenderTarget;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Vision|Roof Fade")
	TSoftObjectPtr<UMaterialInterface> DefaultShadowProxyMaterial;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Vision", meta = (ClampMin = "128", ClampMax = "4096"))
	int32 FallbackRenderTargetSize = 1024;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Vision|Optimization", meta = (ClampMin = "100.0"))
	float SpatialGridCellSize = 800.0f;
};
