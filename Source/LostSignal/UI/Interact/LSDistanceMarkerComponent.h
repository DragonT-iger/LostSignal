#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "LSDistanceMarkerComponent.generated.h"

class UWidgetComponent;
class ULSDistanceMarkerWidget;

// 상호작용 오브젝트 위에 거리 기반으로 뜨는 World 공간 빌보드 마커(원 UI 등).
// MarkerWidgetClass가 미지정이면 기능 전체가 비활성이라 비용이 없다(오브젝트별 opt-in).
UCLASS(ClassGroup=(LS), meta=(BlueprintSpawnableComponent))
class LOSTSIGNAL_API ULSDistanceMarkerComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	ULSDistanceMarkerComponent();

	// 거리와 무관하게 강제로 숨긴다(예: 룻박스가 열리면 true).
	void SetMarkerSuppressed(bool bInSuppressed);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 아트가 BP에서 지정하는 빌보드 위젯 클래스. 미지정이면 마커 비활성.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Interact|Marker")
	TSubclassOf<ULSDistanceMarkerWidget> MarkerWidgetClass;

	// 오브젝트 루트 기준 위젯 표시 높이 오프셋.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Interact|Marker")
	FVector WidgetOffset = FVector(0.0f, 0.0f, 120.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Interact|Marker")
	FVector2D DrawSize = FVector2D(64.0f, 64.0f);

	// 이 거리를 넘으면 완전히 숨긴다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Interact|Marker", meta=(ClampMin="0.0"))
	float MaxVisibleDistance = 3000.0f;

	// 이 거리 이하에서는 완전 불투명. FadeInDistance~MaxVisibleDistance 구간에서 알파가 1→0으로 보간된다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Interact|Marker", meta=(ClampMin="0.0"))
	float FadeInDistance = 1500.0f;

	// 근거리 스케일. 거리 비율 0(가까움)에서 적용.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Interact|Marker", meta=(ClampMin="0.01"))
	float NearScale = 1.0f;

	// 원거리 스케일. 거리 비율 1(최대 거리)에서 적용. 기본은 멀수록 크게 해 가독성 확보.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Interact|Marker", meta=(ClampMin="0.01"))
	float FarScale = 1.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Interact|Marker")
	bool bFaceCamera = true;

	// 마커 갱신 주기(초). 매 프레임 대신 이 간격으로 거리/페이드/빌보딩을 갱신해 비용을 낮춘다. 0이면 매 프레임.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Interact|Marker", meta=(ClampMin="0.0"))
	float MarkerUpdateInterval = 0.1f;

private:
	void CreateWidgetComponent();
	void ConfigureWidgetComponent();
	void UpdateMarker(APlayerController* LocalPlayerController);
	void SetMarkerVisible(bool bShouldBeVisible);
	APlayerController* FindLocalPlayerController() const;

	UPROPERTY(Transient)
	TObjectPtr<UWidgetComponent> MarkerWidgetComponent;

	bool bMarkerVisible = false;
	bool bSuppressed = false;
};
