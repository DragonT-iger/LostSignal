#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Skills/LSSkillAreaTypes.h"
#include "LSSkillPreviewComponent.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS(ClassGroup=(LS), meta=(BlueprintSpawnableComponent))
class LOSTSIGNAL_API ULSSkillPreviewComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSSkillPreviewComponent();

	/** MaterialOverride를 지정하면 컴포넌트 소유 재질 대신 그 재질을 쓴다(몬스터 텔레그래프 등). */
	UFUNCTION(BlueprintCallable, Category="LS/Skill|Preview")
	bool BeginAreaPreview(const FLSSkillAreaPreviewSpec& PreviewSpec, UMaterialInterface* MaterialOverride = nullptr);

	UFUNCTION(BlueprintCallable, Category="LS/Skill|Preview")
	void UpdateAreaPreview(const FVector& WorldLocation, const FRotator& WorldRotation);

	UFUNCTION(BlueprintCallable, Category="LS/Skill|Preview")
	void EndAreaPreview();

	/** 위치/회전·기타 파라미터는 그대로 두고 fill 진행도만 갱신(텔레그래프 차징 등). */
	UFUNCTION(BlueprintCallable, Category="LS/Skill|Preview")
	void SetAreaFillAmount(float NewFillAmount);

	UFUNCTION(BlueprintPure, Category="LS/Skill|Preview")
	bool IsAreaPreviewActive() const { return ActivePreviewMesh != nullptr; }

	UFUNCTION(BlueprintPure, Category="LS/Skill|Preview")
	UStaticMeshComponent* GetActivePreviewMesh() const { return ActivePreviewMesh; }

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(Transient, VisibleInstanceOnly, Category="LS/Skill|Preview")
	TObjectPtr<UStaticMeshComponent> ActivePreviewMesh;

	UPROPERTY(Transient, VisibleInstanceOnly, Category="LS/Skill|Preview")
	TObjectPtr<UMaterialInstanceDynamic> ActivePreviewMaterial;

	UPROPERTY(Transient, VisibleInstanceOnly, Category="LS/Skill|Preview")
	FLSSkillAreaPreviewSpec ActivePreviewSpec;

	UPROPERTY(EditDefaultsOnly, Category="LS/Skill|Preview")
	TObjectPtr<UStaticMesh> DefaultPreviewMesh;

	// 범위 프리뷰 기본 재질(Circle=원/부채꼴 공용, Box=박스). 캐릭터 BP에서 매핑. MaterialOverride가 없을 때 사용된다.
	UPROPERTY(EditDefaultsOnly, Category="LS/Skill|Preview")
	TObjectPtr<UMaterialInterface> CircleMaterial;

	UPROPERTY(EditDefaultsOnly, Category="LS/Skill|Preview")
	TObjectPtr<UMaterialInterface> BoxMaterial;

private:
	void ApplyMeshScale();
	void ApplyMaterialParameters(float WorldYaw);
	float ResolveOwnerFootZ(float FallbackZ) const;

	bool bMissingMaterialWarned = false;
};
