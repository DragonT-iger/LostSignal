#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Skills/LSSkillAreaTypes.h"
#include "LSSkillPreviewComponent.generated.h"

class UDecalComponent;
class UMaterialInstanceDynamic;

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

	UFUNCTION(BlueprintPure, Category="LS/Skill|Preview")
	bool IsAreaPreviewActive() const { return ActivePreviewDecal != nullptr; }

	UFUNCTION(BlueprintPure, Category="LS/Skill|Preview")
	UDecalComponent* GetActivePreviewDecal() const { return ActivePreviewDecal; }

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(Transient, VisibleInstanceOnly, Category="LS/Skill|Preview")
	TObjectPtr<UDecalComponent> ActivePreviewDecal;

	UPROPERTY(Transient, VisibleInstanceOnly, Category="LS/Skill|Preview")
	TObjectPtr<UMaterialInstanceDynamic> ActivePreviewMaterial;

	UPROPERTY(Transient, VisibleInstanceOnly, Category="LS/Skill|Preview")
	FLSSkillAreaPreviewSpec ActivePreviewSpec;

private:
	void ApplyDecalSize();
	void ApplyMaterialParameters(float WorldYaw);
};
