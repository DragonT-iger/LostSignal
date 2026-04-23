#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LSStencilMarkerComponent.generated.h"

class UMeshComponent;
class UPrimitiveComponent;

UCLASS(ClassGroup = (Vision), meta = (BlueprintSpawnableComponent))
class LOSTSIGNAL_API ULSStencilMarkerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSStencilMarkerComponent();

protected:
	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnUnregister() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stencil")
	TArray<TObjectPtr<UPrimitiveComponent>> TargetPrimitives;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stencil")
	bool bAutoFindOwnerMeshes = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stencil")
	bool bEnableCustomDepth = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stencil", meta = (ClampMin = "0", ClampMax = "255"))
	int32 StencilValue = 10;

	UFUNCTION(BlueprintCallable, Category = "Stencil")
	void ApplyStencilSettings();

private:
	void GatherTargetMeshComponents(TArray<UMeshComponent*>& OutMeshComponents) const;
	void RestorePreviousSettings();

	TArray<TWeakObjectPtr<UMeshComponent>> AppliedMeshComponents;
	TArray<bool> PreviousRenderCustomDepthStates;
	TArray<int32> PreviousStencilValues;
};
