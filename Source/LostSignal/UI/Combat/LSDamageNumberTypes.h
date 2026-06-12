#pragma once

#include "CoreMinimal.h"
#include "Engine/NetSerialization.h"
#include "LSDamageNumberTypes.generated.h"

USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSDamageNumberPayload
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="LS/UI|Combat")
	float DamageAmount = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="LS/UI|Combat")
	FVector_NetQuantize WorldLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category="LS/UI|Combat")
	bool bCritical = false;
};
