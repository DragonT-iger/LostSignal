#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LSNoiseTypes.generated.h"

/** Gameplay-only noise event consumed by monster sensing. This is independent from audio playback. */
USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSNoiseEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="LS/Noise")
	TObjectPtr<AActor> NoiseInstigator = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="LS/Noise")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category="LS/Noise", meta=(ClampMin="0.0"))
	float RadiusCm = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="LS/Noise")
	FGameplayTag NoiseTag;

	UPROPERTY(BlueprintReadOnly, Category="LS/Noise")
	bool bNotifyMonsterSense = true;
};
