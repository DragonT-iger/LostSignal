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
	// 0 = 칩
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	int32 Item_Type = 0;

	// 등급(Supply/Standard/Precision/Tuning/Prototype/Masterpiece)은 Row Name(Chip_{Grade}_{Func}) 토큰에서 파싱한다.
	// (LSInventorySlotUtils::ResolveItemGradeFromRowName)

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	int32 Item_Max = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	FText Item_Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	int32 Item_Cost = 0;

	// chip (칩 슬롯)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	FString Item_Equipment;

	// 이 칩이 가진 전투 스탯 개수 (Chip_Stat 테이블에서 해당 등급 범위로 스탯 생성)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item|Chip")
	int32 Item_Chip_Status_Count = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item|Chip")
	int32 Item_MemoryCost = 1;

	// 생존 프로토콜 수치 — HP, 스태미나 관련 시너지
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item|Chip")
	int32 Chip_Protocol_Survival = 0;

	// 적재 프로토콜 수치 — 인벤토리, 보호슬롯 관련 시너지
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item|Chip")
	int32 Chip_Protocol_Carrying = 0;

	// 전투 프로토콜 수치 — 몬스터 HP, 스킬 슬롯 관련 시너지
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item|Chip")
	int32 Chip_Protocol_Battle = 0;

	// 탐색 프로토콜 수치 — 미니맵 관련 시너지
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item|Chip")
	int32 Chip_Protocol_Navigation = 0;
};
