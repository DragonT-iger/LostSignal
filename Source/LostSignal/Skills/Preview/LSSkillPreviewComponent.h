#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Skills/LSSkillAreaTypes.h"
#include "LSSkillPreviewComponent.generated.h"

class UMaterialInstanceDynamic;
class UStaticMesh;
class UStaticMeshComponent;

UCLASS(ClassGroup=(LS), meta=(BlueprintSpawnableComponent))
class LOSTSIGNAL_API ULSSkillPreviewComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSSkillPreviewComponent();

	UFUNCTION(BlueprintCallable, Category="LS/Skill|Preview")
	bool BeginAreaPreview(const FLSSkillAreaPreviewSpec& PreviewSpec);

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

private:
	void ApplyMeshScale();
	void ApplyMaterialParameters(float WorldYaw);
	float ResolveOwnerFootZ(float FallbackZ) const;
};
