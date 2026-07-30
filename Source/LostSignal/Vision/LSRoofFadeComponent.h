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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|RoofFade")
	TArray<TObjectPtr<UPrimitiveComponent>> TargetPrimitives;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|RoofFade")
	bool bUseExistingMaterials = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|RoofFade")
	TObjectPtr<UMaterialInterface> RoofFadeMaterialOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|RoofFade|ShadowProxy")
	bool bCreateShadowProxyMesh = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|RoofFade|ShadowProxy")
	bool bUseSourceMaterialsForShadowProxy = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|RoofFade|ShadowProxy")
	TObjectPtr<UMaterialInterface> ShadowProxyMaterialOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|RoofFade|Trigger")
	bool bUseTriggerVolume = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|RoofFade|Trigger")
	bool bUsePlayerTrigger = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|RoofFade|Trigger")
	bool bUseMouseTrigger = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|RoofFade|Trigger")
	bool bAutoCreateTriggerVolume = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LS/Vision|RoofFade|Trigger")
	TObjectPtr<UBoxComponent> TriggerVolume;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|RoofFade|Trigger")
	FVector TriggerCenterOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|RoofFade|Trigger")
	float TriggerGroundZ = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|RoofFade|Trigger")
	float MouseProjectionPlaneZ = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|RoofFade|Trigger")
	FVector TriggerExtentScale = FVector(1.0f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|RoofFade|Trigger")
	FVector TriggerExtentPadding = FVector(0.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|RoofFade|Trigger")
	bool bDrawDebugTriggerVolume = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|RoofFade|Trigger")
	FColor TriggerDebugColor = FColor::Cyan;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|RoofFade|Trigger", meta = (ClampMin = "0.0"))
	float TriggerDebugThickness = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|RoofFade")
	FVector FadeCenterOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|RoofFade", meta = (ClampMin = "0.0"))
	float FadeRadius = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|RoofFade", meta = (ClampMin = "0.0"))
	float FadeWidth = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|RoofFade", meta = (ClampMin = "0.0"))
	float FadeHalfHeight = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|RoofFade", meta = (ClampMin = "0.0"))
	float HeightFadeWidth = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|RoofFade")
	FName FadeEnabledParamName = TEXT("FadeEnabled");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|RoofFade")
	FName FadeCenterParamName = TEXT("FadeCenterWS");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|RoofFade")
	FName PlayerFadeEnabledParamName = TEXT("PlayerFadeEnabled");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|RoofFade")
	FName PlayerFadeCenterParamName = TEXT("PlayerFadeCenterWS");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|RoofFade")
	FName MouseFadeEnabledParamName = TEXT("MouseFadeEnabled");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|RoofFade")
	FName MouseFadeCenterParamName = TEXT("MouseFadeCenterWS");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|RoofFade")
	FName FadeRadiusParamName = TEXT("FadeRadius");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|RoofFade")
	FName FadeWidthParamName = TEXT("FadeWidth");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|RoofFade")
	FName FadeHalfHeightParamName = TEXT("FadeHalfHeight");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision|RoofFade")
	FName HeightFadeWidthParamName = TEXT("FadeHeightFadeWidth");

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LS/Vision|RoofFade")
	TArray<TObjectPtr<UMaterialInstanceDynamic>> RoofFadeMaterialInstances;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LS/Vision|RoofFade|ShadowProxy")
	TArray<TObjectPtr<UStaticMeshComponent>> ShadowProxyMeshComponents;

	UFUNCTION(BlueprintCallable, Category = "LS/Vision|RoofFade")
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

	// Finds the local player pawn that should drive the cylinder mask center.
	APawn* ResolveLocalPlayerPawn() const;

	// Finds the local player controller used for mouse world projection.
	APlayerController* ResolveLocalPlayerController() const;

	// Projects the current mouse position onto a world Z plane for roof interaction checks.
	bool ResolveMouseWorldPoint(FVector& OutMouseWorldPoint) const;

	// Tests whether the current mouse XY lies within the trigger box footprint.
	bool IsMouseInsideTriggerXY() const;

	// Pushes the current player/mouse fade sources into every bound MID.
	void ApplyFadeParameters(
		float EnabledValue,
		float PlayerEnabledValue,
		const FVector& PlayerFadeCenterWS,
		float MouseEnabledValue,
		const FVector& MouseFadeCenterWS) const;

	TArray<TWeakObjectPtr<UStaticMeshComponent>> ShadowSourceMeshComponents;
	TArray<bool> ShadowSourceCastShadowStates;
};
