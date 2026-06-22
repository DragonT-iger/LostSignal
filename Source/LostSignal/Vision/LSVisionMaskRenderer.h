#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Vision/LSVisionTypes.h"
#include "LSVisionMaskRenderer.generated.h"

class UTexture2D;
class UTextureRenderTarget2D;

UCLASS()
class LOSTSIGNAL_API ALSVisionMaskRenderer : public AActor
{
	GENERATED_BODY()

public:
	ALSVisionMaskRenderer();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision")
	TObjectPtr<UTextureRenderTarget2D> VisibilityMaskRenderTarget;

	// 시야 경계(근접 원/콘)의 부드러운 페이드 폭(월드 유닛). 클수록 경계가 넓게 흐려진다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LS/Vision", meta = (ClampMin = "0.0"))
	float FeatherWidth = 70.0f;

	// 시야 밖 영역에 기록되는 색(머티리얼이 어둡게 처리할 때 사용하는 틴트).
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LS/Vision")
	FLinearColor HiddenColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);

	// 크리스프 경계의 텍셀 계단을 깨는 디더 노이즈 텍스쳐(Wrap 권장). 미지정 시 경계 노이즈 없음.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LS/Vision")
	TObjectPtr<UTexture2D> EdgeNoiseTexture;

	// 노이즈 텍스쳐의 월드 타일링(1/유닛). 클수록 무늬가 잘아져 알갱이(스티플)처럼 보인다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LS/Vision", meta = (ClampMin = "0.0"))
	float EdgeNoiseScale = 0.05f;

	// 경계를 흔드는 폭(월드 유닛). feather와 독립이라 FeatherWidth=0이어도 디더가 동작한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LS/Vision", meta = (ClampMin = "0.0"))
	float EdgeNoiseWidth = 40.0f;

	// 오브젝트(벽) 경계 페더 = FeatherWidth × 이 값. 0=크리스프(정확), 1=열린 경계와 동일. 살짝이면 0.25 정도.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "LS/Vision", meta = (ClampMin = "0.0"))
	float OccluderFeatherScale = 0.25f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LS/Vision")
	FLSVisionPolygonData LastRenderedPolygon;

	void RequestMaskUpdate(const FLSVisionPolygonData& PolygonData);

	UTextureRenderTarget2D* GetVisibilityMaskRenderTarget() const
	{
		return VisibilityMaskRenderTarget;
	}
};
