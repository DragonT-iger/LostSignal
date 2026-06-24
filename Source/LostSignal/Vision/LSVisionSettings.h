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

	// true면 시야 평면 Z를 로컬 플레이어 발 높이 기준으로 잡는다(발 높이 + OccluderSliceHeight). false면 OccluderSliceHeight를 절대 월드 Z로 사용.
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "LS/Vision|Occluder")
	bool bSliceHeightFromPlayer = true;

	// 오클루더 콜리전을 자를 시야 평면 높이. bSliceHeightFromPlayer=true면 "플레이어 발 위로의 오프셋", false면 "절대 월드 Z".
	// 바닥 면과 정확히 일치하면 단면이 degenerate 되므로 살짝 위로 둔다(예: 8). 평면 게임이라 거의 변하지 않아 재슬라이스 비용은 무시 수준.
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "LS/Vision|Occluder")
	float OccluderSliceHeight = 8.0f;

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

	// 마스크 샘플 좌표를 픽셀 월드 노멀 방향으로 미는 강도(픽셀 높이에 비례). 솟아오른/기울어진 면에서 시야 경계가
	// 표면을 일자로 자르는 것을 완화해 경계가 높이를 타고 오르게 한다. 바닥(노멀 ≈+Z, 높이 ≈0)은 사실상 불변.
	// 머티리얼: offset.xy = WorldNormal.XY × (WorldPos.Z − GroundZ) × 이 값. GroundZ는 MaskOriginWS.Z(플레이어 발 높이).
	// 높이에 곱해지는 비율이라 단위는 대략 0.1~0.5 범위. 0=보정 없음(기존 동작).
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "LS/Vision|Projection", meta = (ClampMin = "0.0"))
	float SurfaceNormalPush = 0.0f;
};
