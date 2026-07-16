#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "LSHitFlashComponent.generated.h"

class UAbilitySystemComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class USkeletalMeshComponent;
struct FOnAttributeChangeData;

/**
 * 피격 순간 메시 오버레이 머테리얼의 림 파라미터를 켰다가 짧게 페이드하는 코스메틱 컴포넌트.
 * 오버레이 슬롯은 컴포넌트당 1개뿐이라 기존 아웃라인 오버레이 머테리얼을 MID로 감싸 파라미터만 조작한다
 * (아웃라인 머테리얼 쪽에 HitFlashIntensity로 게이트되는 림 블록이 있어야 실제로 보인다).
 * 체력 감소는 어트리뷰트 리플리케이션으로 모든 머신에서 델리게이트가 발화하므로 authority 체크 없이 로컬 재생한다.
 */
UCLASS(ClassGroup=(LS), meta=(BlueprintSpawnableComponent))
class LOSTSIGNAL_API ULSHitFlashComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSHitFlashComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 플래시 강도가 1→0으로 떨어지는 시간(초).
	UPROPERTY(EditDefaultsOnly, Category="LS/Combat|HitFlash", meta=(ClampMin="0.01"))
	float FlashDuration = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat|HitFlash")
	FLinearColor FlashColor = FLinearColor(1.0f, 0.08f, 0.05f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat|HitFlash")
	FName HitFlashIntensityParamName = TEXT("HitFlashIntensity");

	UPROPERTY(EditDefaultsOnly, Category="LS/Combat|HitFlash")
	FName HitFlashColorParamName = TEXT("HitFlashColor");

private:
	void CreateOverlayMaterialInstance();
	void BindToOwnerASC();
	void UnbindFromOwnerASC();
	void HandleCurrentHealthChanged(const FOnAttributeChangeData& ChangeData);
	void StartFlash();
	void SetFlashIntensity(float Intensity) const;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> OverlayMaterialInstance;

	// EndPlay 복원용 — MID로 교체하기 전 오버레이 머테리얼(애셋 폴백 포함 해석값).
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> OriginalOverlayMaterial;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> TargetMeshComponent;

	TWeakObjectPtr<UAbilitySystemComponent> ObservedASC;
	FDelegateHandle CurrentHealthChangedHandle;
	float FlashTimeRemaining = 0.0f;
};
