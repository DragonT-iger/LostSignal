#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LSArmorRow.generated.h"

USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSArmorRow : public FTableRowBase
{
	GENERATED_BODY()

	// 아이템 이름 출력용 텍스트
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	FText Item_Text;

	// 2 = 방어구
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	int32 Item_Type = 2;

	// Supply / Standard / Presision / Tuning / Prototype / Masterpiece
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	FString Item_Grade;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	int32 Item_Max = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	FText Item_Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	int32 Item_Cost = 0;

	// Processor=머리, Core=몸, Actuator=손, Frame=발
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	FString Item_Equipment;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item|Stats")
	float Item_Health = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item|Stats")
	float Item_Defense = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item|Stats")
	float Item_Recovery = 0.0f;
};
