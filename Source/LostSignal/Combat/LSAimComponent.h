#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LSAimComponent.generated.h"

class APlayerController;

UCLASS(ClassGroup=(LS), meta=(BlueprintSpawnableComponent))
class LOSTSIGNAL_API ULSAimComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSAimComponent();

	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	void UpdateFacing(float DeltaSeconds);

	UFUNCTION(BlueprintPure, Category="LS/Combat")
	bool GetAimPoint(FVector& OutAimPoint) const;

	UFUNCTION(BlueprintPure, Category="LS/Combat")
	FVector GetAimDirection() const;

private:
	UPROPERTY(EditAnywhere, Category="LS/Combat", meta=(ClampMin="0.0"))
	float AimPlaneHeightOffset = 50.0f;

	UPROPERTY(EditAnywhere, Category="LS/Combat", meta=(ClampMin="0.0"))
	float MouseFacingInterpSpeed = 15.0f;

	APlayerController* ResolvePlayerController() const;
};
