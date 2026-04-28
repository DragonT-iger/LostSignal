#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LSPlayerXRayComponent.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;
class USkeletalMeshComponent;

UCLASS(ClassGroup = (Vision), meta = (BlueprintSpawnableComponent))
class LOSTSIGNAL_API ULSPlayerXRayComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSPlayerXRayComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|XRay")
	TObjectPtr<UMaterialInterface> OverlayMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|XRay")
	bool bOnlyLocallyControlled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|XRay")
	float DepthBias = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|XRay")
	float XRayOpacity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|XRay")
	FLinearColor XRayColor = FLinearColor(0.2f, 0.9f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|XRay")
	FName DepthBiasParamName = TEXT("DepthBias");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|XRay")
	FName XRayOpacityParamName = TEXT("XRayOpacity");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|XRay")
	FName XRayColorParamName = TEXT("XRayColor");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LS/Vision|XRay")
	TObjectPtr<USkeletalMeshComponent> OverlayMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LS/Vision|XRay")
	TArray<TObjectPtr<UMaterialInstanceDynamic>> OverlayMaterialInstances;

private:
	USkeletalMeshComponent* ResolveSourceMeshComponent() const;
	void CreateOverlayMeshComponent();
	void RefreshOverlayVisibility();
	void UpdateOverlayMaterialParameters();

	bool bLastOverlayVisible = false;
};
