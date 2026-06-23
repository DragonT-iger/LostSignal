#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "LSVisionSettings.generated.h"

class ALSVisionMaskRenderer;
class UMaterialInterface;
class UTexture2D;
class UTextureRenderTarget2D;

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "LS Vision"))
class LOSTSIGNAL_API ULSVisionSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "LS/Vision")
	TSoftClassPtr<ALSVisionMaskRenderer> MaskRendererClass;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "LS/Vision")
	TSoftObjectPtr<UTextureRenderTarget2D> VisibilityMaskRenderTarget;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "LS/Vision|RoofFade")
	TSoftObjectPtr<UMaterialInterface> DefaultShadowProxyMaterial;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "LS/Vision", meta = (ClampMin = "128", ClampMax = "4096"))
	int32 FallbackRenderTargetSize = 1024;

	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "LS/Vision|Optimization", meta = (ClampMin = "100.0"))
	float SpatialGridCellSize = 800.0f;

	// 시야 경계(근접 원/바깥 경계/오브젝트 경계)의 페이드 폭(월드 유닛). 클수록 경계가 부드럽게 퍼진다.
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "LS/Vision|Edge", meta = (ClampMin = "0.0"))
	float FeatherWidth = 70.0f;

	// 시야 밖 영역에 기록되는 색(머티리얼이 어둡게 처리할 때 사용하는 틴트).
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "LS/Vision")
	FLinearColor HiddenColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);

	// 크리스프 경계의 텍셀 계단을 깨는 디더 노이즈 텍스쳐(Wrap 권장). 미지정 시 경계 노이즈 없음.
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "LS/Vision|EdgeNoise")
	TSoftObjectPtr<UTexture2D> EdgeNoiseTexture;

	// 노이즈 텍스쳐의 월드 타일링(1/유닛). 클수록 무늬가 잘아져 알갱이(스티플)처럼 보인다.
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "LS/Vision|EdgeNoise", meta = (ClampMin = "0.0"))
	float EdgeNoiseScale = 0.05f;

	// 경계를 흔드는 폭(월드 유닛). feather와 독립이라 FeatherWidth=0이어도 디더가 동작한다.
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "LS/Vision|EdgeNoise", meta = (ClampMin = "0.0"))
	float EdgeNoiseWidth = 40.0f;

	// 오브젝트(벽) 경계 페더 = FeatherWidth × 이 값. 0=크리스프(정확), 1=열린 경계와 동일. 살짝이면 0.25 정도.
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "LS/Vision|Edge", meta = (ClampMin = "0.0"))
	float OccluderFeatherScale = 0.25f;
};
