#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LSItemRow.generated.h"

USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSItemRow : public FTableRowBase
{
	GENERATED_BODY()

	// 아이템 이름 출력용 텍스트
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	FText Item_Text;

	// 3=일반(판매용), 4~9=소모품(퀵슬롯), 11~19=퀘스트, 20~=재료(합성/강화)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	int32 Item_Type = 3;

	// 등급(Supply/Standard/Precision/Tuning/Prototype/Masterpiece)은 Row Name 끝 토큰에서 파싱한다.
	// (LSInventorySlotUtils::ResolveItemGradeFromRowName)

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	int32 Item_Max = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	FText Item_Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	int32 Item_Cost = 0;

	// --- 소모품 전용 필드 (Item_Type 4~9일 때만 사용) ---

	// 영향을 받는 스탯 또는 UI 이름 (예: UI_Health, UI_Stamina)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item|Consumable")
	FName Item_Target_Status;

	// 스탯 변화 수치
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item|Consumable")
	float Item_Target_Status_Value = 0.0f;

	// 적용할 상태이상 이름 (회복 또는 디버프)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item|Consumable")
	FName Item_Target_Buff;

	// 상태이상 수치 (예: 0=회복, 100=디버프)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item|Consumable")
	float Item_Target_Buff_Value = 0.0f;

	// 아이템 사용 시작까지 걸리는 시간 (초, 배그 붕대/구급상자 차이)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item|Consumable")
	float Item_Cast_Time = 0.0f;

	// 아이템 효과 지속 시간 (초, 0이면 즉시 적용)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item|Consumable")
	float Item_Duration = 0.0f;
};
