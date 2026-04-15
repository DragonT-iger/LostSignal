#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Vision/LSVisionTypes.h"
#include "LSVisionMaskRenderer.generated.h"

class UTextureRenderTarget2D;

UCLASS()
class LOSTSIGNAL_API ALSVisionMaskRenderer : public AActor
{
	GENERATED_BODY()

public:
	ALSVisionMaskRenderer();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vision")
	TObjectPtr<UTextureRenderTarget2D> VisibilityMaskRenderTarget;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Vision")
	FLSVisionPolygonData LastRenderedPolygon;

	void RequestMaskUpdate(const FLSVisionPolygonData& PolygonData);

	UTextureRenderTarget2D* GetVisibilityMaskRenderTarget() const
	{
		return VisibilityMaskRenderTarget;
	}
};
