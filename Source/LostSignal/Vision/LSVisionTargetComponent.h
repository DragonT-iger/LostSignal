#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LSVisionTargetComponent.generated.h"

class UPrimitiveComponent;

UCLASS(ClassGroup = (Vision), meta = (BlueprintSpawnableComponent))
class LOSTSIGNAL_API ULSVisionTargetComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSVisionTargetComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision")
	TArray<TObjectPtr<UPrimitiveComponent>> RenderPrimitives;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision")
	bool bUseOwnerPrimitiveComponents = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision")
	bool bHideWhenNotVisible = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LS/Vision")
	TArray<FVector> VisibilitySampleOffsets;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LS/Vision")
	bool bIsLocallyVisible = true;

	UFUNCTION(BlueprintCallable, Category = "LS/Vision")
	void SetLocallyVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "LS/Vision")
	void GatherVisibilitySamplePoints(TArray<FVector>& OutSamplePoints) const;

private:
	void GatherRenderPrimitives(TArray<UPrimitiveComponent*>& OutPrimitives) const;
};
