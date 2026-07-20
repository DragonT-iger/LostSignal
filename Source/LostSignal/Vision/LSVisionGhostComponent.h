#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "LSVisionGhostComponent.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;
class USkeletalMeshComponent;

UCLASS(Transient)
class ULSVisionGhostMeshComponent final : public UPoseableMeshComponent
{
	GENERATED_BODY()

public:
	virtual void GetDefaultMaterialSlotsOverlayMaterial(
		TArray<TObjectPtr<UMaterialInterface>>& OutMaterialSlotOverlayMaterials) const override;

protected:
	virtual UMaterialInterface* GetDefaultOverlayMaterial() const override;
};

/**
 * 적이 로컬 시야에서 사라지는 순간, 마지막 위치·포즈에 고정된 실루엣(잔상) 메쉬를 남기고
 * 서서히 페이드 아웃시키는 로컬 전용 코스메틱 컴포넌트.
 * ULSVisionTargetComponent::OnLocalVisibilityChanged를 구독해 동작하며, 잔상 메쉬는
 * 어떤 프리미티브에도 attach하지 않아(비부착) VisionTarget의 SetVisibility 전파에서 격리된다.
 */
UCLASS(ClassGroup = (Vision), meta = (BlueprintSpawnableComponent))
class LOSTSIGNAL_API ULSVisionGhostComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSVisionGhostComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	// 잔상 실루엣 머티리얼. 미할당 시 기능 비활성(원본 머티리얼 폴백 금지 — 살아있는 적으로 오독됨).
	UPROPERTY(EditDefaultsOnly, Category = "LS/Vision|Ghost")
	TObjectPtr<UMaterialInterface> GhostMaterial;

	UPROPERTY(EditAnywhere, Category = "LS/Vision|Ghost", meta = (ClampMin = "0.01"))
	float FadeDuration = 1.5f;

	// 페이드 시작 전 잔상이 완전 불투명으로 유지되는 시간.
	UPROPERTY(EditAnywhere, Category = "LS/Vision|Ghost", meta = (ClampMin = "0.0"))
	float FadeStartDelay = 1.0f;

	UPROPERTY(EditAnywhere, Category = "LS/Vision|Ghost")
	FLinearColor GhostColor = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);

	UPROPERTY(EditAnywhere, Category = "LS/Vision|Ghost")
	FName GhostOpacityParamName = TEXT("GhostOpacity");

	UPROPERTY(EditAnywhere, Category = "LS/Vision|Ghost")
	FName GhostColorParamName = TEXT("GhostColor");

	// 본체 메쉬가 이 시간(초) 안에 실제 렌더된 적이 있어야 잔상을 남긴다 (시작부터 시야 밖인 적 배제).
	UPROPERTY(EditAnywhere, Category = "LS/Vision|Ghost", meta = (ClampMin = "0.0"))
	float RecentlyRenderedTolerance = 0.2f;

	// true면 사망한 적은 잔상을 남기지 않는다.
	UPROPERTY(EditAnywhere, Category = "LS/Vision|Ghost")
	bool bSuppressWhenDead = false;

	// PP 시야 어둠에서 잔상이 안 읽힐 때 스텐실 예외 처리용 (M_PP_PlayerVision 수정과 세트로 사용).
	UPROPERTY(EditAnywhere, Category = "LS/Vision|Ghost")
	bool bEnableCustomDepth = false;

	UPROPERTY(EditAnywhere, Category = "LS/Vision|Ghost", meta = (ClampMin = "0", ClampMax = "255", EditCondition = "bEnableCustomDepth"))
	int32 CustomDepthStencilValue = 0;

private:
	USkeletalMeshComponent* ResolveSourceMeshComponent() const;
	void CreateGhostMeshComponent();
	void CreateGhostMaterialInstances(const USkeletalMeshComponent* SourceMeshComponent);
	void HandleLocalVisibilityChanged(bool bLocallyVisible);
	void BeginGhostFade();
	void ClearGhostImmediate();
	void ApplyGhostOpacity(float Opacity);

	UPROPERTY(Transient)
	TObjectPtr<ULSVisionGhostMeshComponent> GhostMeshComponent;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> GhostMaterialInstances;

	FDelegateHandle VisibilityChangedHandle;
	float FadeElapsed = 0.0f;
	bool bGhostActive = false;
	bool bWarnedMissingGhostMaterial = false;
};
