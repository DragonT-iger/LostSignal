#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UObject/SoftObjectPtr.h"
#include "LSInteractMarkerSettings.generated.h"

class ULSDistanceMarkerWidget;
class UMaterialInterface;

/** 거리 기반 빌보드 마커가 공유하는 전역 위젯 설정. 마커를 켠 모든 상호작용 오브젝트가 이 UI를 받아 쓴다. */
UCLASS(config=Game, defaultconfig, meta=(DisplayName="LS Interact Marker Settings"))
class LOSTSIGNAL_API ULSInteractMarkerSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(config, EditAnywhere, Category="LS/Interact|Marker")
	TSoftClassPtr<ULSDistanceMarkerWidget> DistanceMarkerWidgetClass;

	// 위젯 컴포넌트(바깥 3D 쿼드)에 물릴 렌더 머티리얼. "Disable Depth Test"를 켠 3D 위젯 패스스루
	// 머티리얼을 넣으면 마커가 벽/건물에 가려지지 않고 항상 위에 그려진다. 비우면 엔진 기본(뎁스 테스트 ON).
	UPROPERTY(config, EditAnywhere, Category="LS/Interact|Marker")
	TSoftObjectPtr<UMaterialInterface> DistanceMarkerWidgetMaterial;
};
