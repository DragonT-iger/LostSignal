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
	// 켤 때의 비용: 발 높이가 1유닛 넘게 변하면 등록된 모든 오클루더의 단면을 다시 계산하고(SetRuntimeSliceZ),
	// 그 과정에서 세그먼트 토폴로지 버전이 올라 폴리곤 재solve까지 강제된다. 점프·낙하 중에는 시야 갱신마다 발생한다.
	// 평면 레벨에서는 이득이 없으므로 기본은 끈다 — 다층 지형이 필요해지면 재슬라이스 스로틀링을 함께 설계해야 한다.
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "LS/Vision|Occluder")
	bool bSliceHeightFromPlayer = false;

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

	// 마스크 샘플 좌표를 픽셀 월드 노멀 XY 방향으로 미는 거리(월드 유닛). 벽 앞면 샘플을 플레이어 쪽(시야 안)으로 밀어,
	// 오클루더 벽이 자기 발자국 경계를 샘플해 통째로 어두워지는 것을 막는다. 높이와 무관해 플레이어 수직 이동에 불변.
	// 머티리얼: offset.xy = WorldNormal.XY × 이 값. 바닥(노멀 ≈+Z, XY≈0)은 불변, 수직 벽일수록 크게 밀린다.
	// 단위는 월드 유닛 — 벽 두께를 넘어 시야 안으로 확실히 들어갈 만큼(대략 20~50). 0=보정 없음(벽이 까매질 수 있음).
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "LS/Vision|Projection", meta = (ClampMin = "0.0"))
	float SurfaceNormalPush = 30.0f;
};
