#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Skills/LSSkillAreaTypes.h"
#include "LSSkillAreaPreviewActor.generated.h"

class UMaterialInstanceDynamic;
class USceneComponent;
class UStaticMeshComponent;

UCLASS()
class LOSTSIGNAL_API ALSSkillAreaPreviewActor : public AActor
{
	GENERATED_BODY()

public:
	ALSSkillAreaPreviewActor();

	UFUNCTION(BlueprintCallable, Category="LS/Skill|Preview")
	void SetAreaSpec(const FLSSkillAreaPreviewSpec& InSpec);

	UFUNCTION(BlueprintCallable, Category="LS/Skill|Preview")
	void SetPreviewTransform(const FVector& WorldLocation, const FRotator& WorldRotation);

	UFUNCTION(BlueprintCallable, Category="LS/Skill|Preview")
	void SetPreviewVisible(bool bVisible);

	UFUNCTION(BlueprintPure, Category="LS/Skill|Preview")
	const FLSSkillAreaPreviewSpec& GetAreaSpec() const { return AreaSpec; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Skill|Preview")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Skill|Preview")
	TObjectPtr<UStaticMeshComponent> AreaMeshComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Preview", meta=(ClampMin="1.0"))
	float MeshBaseSize = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Preview")
	FLSSkillAreaPreviewSpec AreaSpec;

	UPROPERTY(Transient, VisibleInstanceOnly, Category="LS/Skill|Preview")
	TObjectPtr<UMaterialInstanceDynamic> PreviewMaterialInstance;

private:
	void ApplyAreaSpec();
	void ApplyMeshScale();
	void ApplyMaterialParameters();
	void EnsureMaterialInstance();
};
