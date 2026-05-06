#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LSChipRow.generated.h"

USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSChipRow : public FTableRowBase
{
	GENERATED_BODY()

	// 아이템 이름 출력용 텍스트
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	FText Item_Text;

	// 아이콘 에셋 경로
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	FString Icon_Path;

	// 0 = 칩
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	int32 Item_Type = 0;

	// 0=보급, 1=표준, 2=정밀, 3=튜닝, 4=프로토타입, 5=마스터피스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	int32 Item_Grade = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	int32 Item_Max = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	FText Item_Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	int32 Item_Cost = 0;

	// chip (칩 슬롯)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	FString Item_Equipment;

	// 칩 UI 슬롯 1 (예: UI_Health, UI_Stamina)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item|Chip")
	FString Item_Chip_UI_1;

	// 칩 UI 슬롯 2
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item|Chip")
	FString Item_Chip_UI_2;

	// 칩 UI 슬롯 3
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item|Chip")
	FString Item_Chip_UI_3;

	// 이 칩이 가진 전투 스탯 개수 (Chip_Stat 테이블에서 해당 등급 범위로 스탯 생성)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item|Chip")
	int32 Item_Chip_Status_Count = 0;
};
