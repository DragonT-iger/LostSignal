#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "LSDistanceMarkerComponent.generated.h"

class UWidgetComponent;
class ULSDistanceMarkerWidget;
class UMaterialInterface;

// 상호작용 오브젝트 위에 거리 기반으로 뜨는 World 공간 빌보드 마커(원 UI 등).
// 이 컴포넌트 자체가 감지용 콜라이더(InteractMarker 채널)이며, 캐릭터의 MarkerActivationSphere가
// 오버랩할 때만 활성(틱)된다. MarkerWidgetClass 미지정이면 기능 전체 비활성(무비용, 오브젝트별 opt-in).
UCLASS(ClassGroup=(LS), meta=(BlueprintSpawnableComponent))
class LOSTSIGNAL_API ULSDistanceMarkerComponent : public USphereComponent
{
	GENERATED_BODY()

public:
	ULSDistanceMarkerComponent();

	// 캐릭터의 활성화 스피어 오버랩 begin/end에서 호출된다. 근접 시에만 마커가 갱신·표시된다.
	void SetActivatedByProximity(bool bInActivated);

	// 거리와 무관하게 강제로 숨긴다(예: 룻박스가 열리면 true). 억제되면 감지 콜리전도 꺼 완전 idle.
	void SetMarkerSuppressed(bool bInSuppressed);

	// 이 오브젝트가 마커를 표시할지. 파생 액터가 생성자에서 켠다(예: 룻박스). BeginPlay 전에 설정해야 한다.
	void SetMarkerEnabled(bool bInEnabled) { bEnableMarker = bInEnabled; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 마커 표시 여부(opt-in). 위젯 클래스는 전역 ULSInteractMarkerSettings에서 받아 쓴다.
	// 파생 액터가 C++에서 기본값을 켜고, 필요하면 BP에서 오브젝트별로 덮어쓸 수 있다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Interact|Marker")
	bool bEnableMarker = false;

	// 오브젝트 루트 기준 위젯 표시 높이 오프셋.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Interact|Marker")
	FVector WidgetOffset = FVector(0.0f, 0.0f, 80.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Interact|Marker")
	FVector2D DrawSize = FVector2D(32.0f, 32.0f);

	// 감지용 콜라이더 반경. 활성화 거리는 캐릭터 스피어 반경이 결정하므로 작게 두면 된다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Interact|Marker", meta=(ClampMin="1.0"))
	float DetectionRadius = 48.0f;

	// 이 거리를 넘으면 완전히 숨긴다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Interact|Marker", meta=(ClampMin="0.0"))
	float MaxVisibleDistance = 800.0f;

	// 이 거리 이하에서는 완전 불투명. FadeInDistance~MaxVisibleDistance 구간에서 알파가 1→0으로 보간된다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Interact|Marker", meta=(ClampMin="0.0"))
	float FadeInDistance = 600.0f;

	// 근거리 스케일. 거리 비율 0(가까움)에서 적용.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Interact|Marker", meta=(ClampMin="0.01"))
	float NearScale = 1.0f;

	// 원거리 스케일. 거리 비율 1(최대 거리)에서 적용. 기본은 멀수록 크게 해 가독성 확보.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Interact|Marker", meta=(ClampMin="0.01"))
	float FarScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Interact|Marker")
	bool bFaceCamera = true;

	// 마커 갱신 주기(초). 매 프레임 대신 이 간격으로 거리/페이드/빌보딩을 갱신해 비용을 낮춘다. 0이면 매 프레임.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Interact|Marker", meta=(ClampMin="0.0"))
	float MarkerUpdateInterval = 0.1f;

private:
	bool IsMarkerFeatureEnabled() const;
	void ConfigureDetectionCollision();
	void CreateWidgetComponent();
	void ConfigureWidgetComponent();
	void RefreshActiveState();
	void UpdateMarker(APlayerController* LocalPlayerController);
	void SetMarkerVisible(bool bShouldBeVisible);
	APlayerController* FindLocalPlayerController() const;

	UPROPERTY(Transient)
	TObjectPtr<UWidgetComponent> MarkerWidgetComponent;

	// 전역 설정에서 로드한 마커 위젯 클래스(BeginPlay에서 1회 해석).
	UPROPERTY(Transient)
	TSubclassOf<ULSDistanceMarkerWidget> ResolvedMarkerWidgetClass;

	// 전역 설정에서 로드한 위젯 렌더 머티리얼(선택). 뎁스 테스트 off 등에 사용. 없으면 엔진 기본.
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> ResolvedMarkerMaterial;

	bool bMarkerVisible = false;
	bool bSuppressed = false;
	bool bActivatedByProximity = false;
	bool bWarnedMissingWidgetClass = false;
};
