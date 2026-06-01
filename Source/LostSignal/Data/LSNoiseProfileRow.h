#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "LSNoiseProfileRow.generated.h"

/** DataTable row for character gameplay noise. Radius is authored in meters by design. */
USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSNoiseProfileRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Noise")
	FGameplayTag NoiseTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Noise", meta=(ClampMin="0.0"))
	float RadiusMeters = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Noise", meta=(ClampMin="0.0"))
	float EmitIntervalSeconds = 0.0f;
};
