#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LSVisionTargetComponent.generated.h"

class UPrimitiveComponent;

// 로컬 시야 가시성이 실제로 바뀔 때만 발화 (C++ 전용, 잔상 등 코스메틱 연출 구독용)
DECLARE_MULTICAST_DELEGATE_OneParam(FLSOnLocalVisibilityChanged, bool /*bLocallyVisible*/);

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

	FLSOnLocalVisibilityChanged OnLocalVisibilityChanged;

	UFUNCTION(BlueprintCallable, Category = "LS/Vision")
	void SetLocallyVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "LS/Vision")
	void GatherVisibilitySamplePoints(TArray<FVector>& OutSamplePoints) const;

private:
	void GatherRenderPrimitives(TArray<UPrimitiveComponent*>& OutPrimitives) const;
};
