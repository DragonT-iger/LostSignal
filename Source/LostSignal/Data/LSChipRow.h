#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LSChipRow.generated.h"

USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSChipRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	FString Item_Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	int32 Item_Type = 0;

	// 0=보급, 1=표준, 2=정밀, 3=튜닝, 4=프로토타입, 5=마스터피스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	int32 Item_Grade = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	int32 Item_Max = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	FString Item_Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	int32 Item_Cost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	FString Item_Equipment;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item|Chip")
	FString Item_Chip_UI_1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item|Chip")
	FString Item_Chip_UI_2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item|Chip")
	FString Item_Chip_UI_3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item|Chip")
	int32 Item_Chip_Status_Count = 0;
};
