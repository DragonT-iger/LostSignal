#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "LSMonsterArchetypeRow.generated.h"

/** DataTable row for a monster archetype's sensing and combat tuning. */
USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSMonsterArchetypeRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/AI")
	FText MonsterName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/AI|Sense", meta=(ClampMin="0.0"))
	float SightRadius = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/AI|Sense", meta=(ClampMin="0.0"))
	float MaxSightRadius = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/AI|Sense", meta=(ClampMin="0.0", ClampMax="180.0"))
	float SightHalfAngleDegrees = 55.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/AI|Sense", meta=(ClampMin="0.0"))
	float HearingRadius = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/AI|Sense", meta=(ClampMin="0.0"))
	float InterestMemorySeconds = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/AI|Sense", meta=(ClampMin="0.0"))
	float SuspicionDecayPerSecond = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combat", meta=(ClampMin="0.0"))
	float LeashDistance = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combat", meta=(ClampMin="0.0"))
	float AlertDuration = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combat", meta=(ClampMin="0.0"))
	float AlertMoveSpeedMultiplier = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combat")
	FGameplayTag DefaultAttackAbilityTag;
};
