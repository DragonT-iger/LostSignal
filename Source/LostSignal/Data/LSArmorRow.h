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

	// 아이템 인덱스

	// 아이콘 에셋 경로
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	FString Icon_Path;

	// 2 = 방어구
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	int32 Item_Type = 2;

	// 0=보급, 1=표준, 2=정밀, 3=튜닝, 4=프로토타입, 5=마스터피스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	int32 Item_Grade = 0;

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
