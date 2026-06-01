#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "UObject/SoftObjectPath.h"
#include "LSMonsterArchetypeRow.generated.h"

/** DataTable row matching the monster planning CSV. */
USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSMonsterArchetypeRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/AI")
	FText Monster_Name_KR;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/AI")
	FSoftObjectPath Monster_Resource_Path;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combat")
	FName Monster_Combat_Type;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Stats", meta=(ClampMin="0.0"))
	float Monster_HP = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Stats", meta=(ClampMin="0.0"))
	float Monster_ATK = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Stats", meta=(ClampMin="0.0"))
	float Monster_DEF = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Stats", meta=(ClampMin="0.0"))
	float Monster_Guard = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/AI|Sense", meta=(ClampMin="0.0"))
	float Sight_Radius = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/AI|Sense", meta=(ClampMin="0.0"))
	float Hearing_Radius = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combat", meta=(ClampMin="0.0"))
	float Patrol_Speed = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combat", meta=(ClampMin="0.0"))
	float Chase_Speed = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Combat")
	FName Action_Group;
};
