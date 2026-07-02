#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "LSCombatBuffTypes.generated.h"

class UTexture2D;

USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSCombatBuffDisplayData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="LS/UI|Combat")
	FGameplayTag BuffTag;

	UPROPERTY(BlueprintReadOnly, Category="LS/UI|Combat")
	TObjectPtr<UTexture2D> IconTexture = nullptr;

	UPROPERTY(BlueprintReadOnly, Category="LS/UI|Combat")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category="LS/UI|Combat")
	FText Description;

	UPROPERTY(BlueprintReadOnly, Category="LS/UI|Combat")
	float RemainingTime = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="LS/UI|Combat")
	float TotalDuration = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="LS/UI|Combat")
	int32 StackCount = 1;
};
