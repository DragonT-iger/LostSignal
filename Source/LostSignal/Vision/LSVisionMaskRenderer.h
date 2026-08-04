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

	// 경계 페더·노이즈·틴트 값의 단일 출처는 ULSVisionSettings(Config)다. 이 액터는 그 값을 다시 선언하지 않고
	// RequestMaskUpdate에서 직접 읽는다 — 예전엔 여기 복제돼 있었지만 서브시스템이 항상 덮어써 편집해도 효과가 없었다.

	// 경계 노이즈 텍스쳐의 런타임 해석 캐시(편집용 노브가 아니다). 설정 쪽은 TSoftObjectPtr이라 디스패치마다
	// 읽으면 프레임당 LoadSynchronous가 되므로, ULSVisionSubsystem이 스폰 시 한 번 해석해 여기 담아둔다.
	// GC가 추적할 수 있도록 UPROPERTY로 유지한다.
	UPROPERTY(VisibleAnywhere, Transient, Category = "LS/Vision")
	TObjectPtr<UTexture2D> EdgeNoiseTexture;

	void RequestMaskUpdate(const FLSVisionPolygonData& PolygonData);

	UTextureRenderTarget2D* GetVisibilityMaskRenderTarget() const
	{
		return VisibilityMaskRenderTarget;
	}
};
