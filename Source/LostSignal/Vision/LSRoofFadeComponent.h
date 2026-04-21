#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LSRoofFadeComponent.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;
class UBoxComponent;
class UMeshComponent;
class UPrimitiveComponent;
class UStaticMeshComponent;
struct FPropertyChangedEvent;

UCLASS(ClassGroup = (Vision), meta = (BlueprintSpawnableComponent))
class LOSTSIGNAL_API ULSRoofFadeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSRoofFadeComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnRegister() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roof Fade")
	TArray<TObjectPtr<UPrimitiveComponent>> TargetPrimitives;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roof Fade")
	bool bUseExistingMaterials = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roof Fade")
	TObjectPtr<UMaterialInterface> RoofFadeMaterialOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roof Fade|Shadow Proxy")
	bool bCreateShadowProxyMesh = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roof Fade|Shadow Proxy")
	bool bUseSourceMaterialsForShadowProxy = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roof Fade|Shadow Proxy")
	TObjectPtr<UMaterialInterface> ShadowProxyMaterialOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roof Fade|Trigger")
	bool bUseTriggerVolume = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roof Fade|Trigger")
	bool bAutoCreateTriggerVolume = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Roof Fade|Trigger")
	TObjectPtr<UBoxComponent> TriggerVolume;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roof Fade|Trigger")
	FVector TriggerCenterOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roof Fade|Trigger")
	FVector TriggerExtentScale = FVector(1.2f, 1.2f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roof Fade|Trigger")
	FVector TriggerExtentPadding = FVector(0.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roof Fade|Trigger")
	bool bDrawDebugTriggerVolume = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roof Fade|Trigger")
	FColor TriggerDebugColor = FColor::Cyan;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roof Fade|Trigger", meta = (ClampMin = "0.0"))
	float TriggerDebugThickness = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roof Fade")
	FVector FadeCenterOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roof Fade", meta = (ClampMin = "0.0"))
	float FadeRadius = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roof Fade", meta = (ClampMin = "0.0"))
	float FadeWidth = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roof Fade", meta = (ClampMin = "0.0"))
	float FadeHalfHeight = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roof Fade", meta = (ClampMin = "0.0"))
	float HeightFadeWidth = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roof Fade", meta = (ClampMin = "0", ClampMax = "255"))
	int32 StencilValue = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roof Fade")
	FName FadeEnabledParamName = TEXT("FadeEnabled");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roof Fade")
	FName FadeCenterParamName = TEXT("FadeCenterWS");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roof Fade")
	FName FadeRadiusParamName = TEXT("FadeRadius");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roof Fade")
	FName FadeWidthParamName = TEXT("FadeWidth");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roof Fade")
	FName FadeHalfHeightParamName = TEXT("FadeHalfHeight");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Roof Fade")
	FName HeightFadeWidthParamName = TEXT("FadeHeightFadeWidth");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Roof Fade")
	TArray<TObjectPtr<UMaterialInstanceDynamic>> RoofFadeMaterialInstances;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Roof Fade|Shadow Proxy")
	TArray<TObjectPtr<UStaticMeshComponent>> ShadowProxyMeshComponents;

	UFUNCTION(BlueprintCallable, Category = "Roof Fade")
	void InitializeFadeMaterials();

private:
	// Collects mesh components that should receive the cylinder-fade MID setup.
	void GatherTargetMeshComponents(TArray<UMeshComponent*>& OutMeshComponents) const;

	// Finds the static mesh component used as the source for auto trigger sizing and shadow proxy duplication.
	UStaticMeshComponent* ResolvePrimaryStaticMeshComponent() const;

	// Creates a trigger box once and keeps it aligned to the source mesh bounds.
	void EnsureTriggerVolume();

	// Rebuilds the trigger box center and extent from the source mesh bounds plus user adjustments.
	void UpdateTriggerVolumeFromSourceMesh();

	// Draws the current trigger volume bounds for editor/game verification.
	void DrawDebugTriggerVolume() const;

	// Creates hidden proxy meshes that only participate in shadow rendering while the visible roof uses fade materials.
	void CreateShadowProxyMeshes();

	// Resolves which opaque material the hidden shadow proxy should use.
	UMaterialInterface* ResolveShadowProxyMaterial() const;

	// Marks the roof meshes in custom depth so stencil-based post effects can identify roof occluders.
	void ApplyCustomDepthStencil() const;

	// Finds the local player pawn that should drive the cylinder mask center.
	APawn* ResolveLocalPlayerPawn() const;

	// Pushes the current cylinder parameters into every bound MID.
	void ApplyFadeParameters(float EnabledValue, const FVector& FadeCenterWS) const;

	TArray<TWeakObjectPtr<UStaticMeshComponent>> ShadowSourceMeshComponents;
	TArray<bool> ShadowSourceCastShadowStates;
};
