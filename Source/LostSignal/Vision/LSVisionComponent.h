#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Vision/LSVisionTypes.h"
#include "LSVisionComponent.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;

UCLASS(ClassGroup = (Vision), meta = (BlueprintSpawnableComponent))
class LOSTSIGNAL_API ULSVisionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSVisionComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void UpdateVisionPolygon();
	bool IsLocalVisionController() const;
	bool IsPointVisibleInCurrentVision(const FVector2D& Point2D) const;
	void UpdateVisionTargets(const FVector2D& VisionOrigin2D);
	// 현재 폴리곤 기준 마스크 파라미터를 등록된 모든 서피스에 푸시한다.
	// 재solve 경로와 "서피스만 새로 등록된" 경로 양쪽에서 호출된다.
	void ApplyVisionParametersToSurfaces(const FVector2D& Forward2D, float SliceZ);
	void DrawDebugVisionRays() const;
	void InitializeLocalVision();
	void ShutdownLocalVision();

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision")
	bool bEnableVision = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision")
	float VisionRadius = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision")
	float HalfFOVDegrees = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision")
	float MaxRayDistance = 2500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision", meta = (ClampMin = "0.001"))
	float UpdateInterval = 0.008f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision", meta = (ClampMin = "0.01"))
	float DivideAngleDegree = 0.5f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LS/Vision")
	FLSVisionPolygonData CurrentPolygon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|Debug")
	bool bDrawDebugRays = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|Debug")
	FColor DebugRayColor = FColor::Yellow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|Debug", meta = (ClampMin = "0.0"))
	float DebugRayDuration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|Debug", meta = (ClampMin = "0.0"))
	float DebugRayThickness = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|Debug")
	float DebugRayZOffset = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LS/Vision")
	TObjectPtr<UMaterialInterface> PostProcessMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision")
	FName VisibilityMaskTextureParamName = TEXT("VisibilityMaskRT");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision")
	FName MaskOriginParamName = TEXT("MaskOriginWS");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision")
	FName MaskExtentParamName = TEXT("MaskExtent");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision")
	FName EnableParamName = TEXT("VisionEnable");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision")
	FName SurfacePushParamName = TEXT("MaskSurfacePush");

private:
	FTimerHandle VisionUpdateTimerHandle;
	TObjectPtr<UMaterialInstanceDynamic> PostProcessMID;
	bool bLocalVisionInitialized = false;

	// MaskRenderer/Subsystem 미바인딩 경고를 매 프레임 스팸하지 않도록 1회만 찍기 위한 상태.
	bool bWarnedMissingMaskRenderer = false;

	// 직전 solve 상태 캐시. 플레이어 포즈·오클루더 토폴로지·활성화 플래그가 모두 그대로면 재계산을 건너뛴다.
	bool bHasSolvedOnce = false;
	bool bLastEnableVision = true;
	int32 LastSolveTopologyVersion = -1;
	FVector2D LastSolveOrigin = FVector2D::ZeroVector;
	FVector2D LastSolveForward = FVector2D::ZeroVector;

	// 서피스 파라미터를 마지막으로 푸시한 시점의 서피스 등록 버전.
	// 폴리곤이 그대로여도 이 값이 밀리면(스트리밍 인 등) 새 서피스에 파라미터를 다시 먹여야 한다.
	int32 LastSurfaceRegistryVersion = -1;
};
