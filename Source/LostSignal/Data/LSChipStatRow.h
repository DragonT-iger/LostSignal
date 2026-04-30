#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LSChipStatRow.generated.h"

USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSChipStatRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Chip|Attack")
	float Chip_Attack_Min = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Chip|Attack")
	float Chip_Attack_Max = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Chip|Attack")
	float Chip_Attack_Speed_Min = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Chip|Attack")
	float Chip_Attack_Speed_Max = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Chip|Attack")
	float Chip_Skill_Haste_Min = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Chip|Attack")
	float Chip_Skill_Haste_Max = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Chip|Attack")
	float Chip_Critical_Rate_Min = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Chip|Attack")
	float Chip_Critical_Rate_Max = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Chip|Attack")
	float Chip_Critical_Damage_Min = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Chip|Attack")
	float Chip_Critical_Damage_Max = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Chip|Attack")
	float Chip_Defense_Penetration_Min = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Chip|Attack")
	float Chip_Defense_Penetration_Max = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Chip|Defense")
	float Chip_Health_Min = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Chip|Defense")
	float Chip_Health_Max = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Chip|Defense")
	float Chip_Defense_Min = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Chip|Defense")
	float Chip_Defense_Max = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Chip|Defense")
	float Chip_Recovery_Min = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Chip|Defense")
	float Chip_Recovery_Max = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Chip|Survival")
	float Chip_Move_Speed_Min = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Chip|Survival")
	float Chip_Move_Speed_Max = 0.0f;
};
