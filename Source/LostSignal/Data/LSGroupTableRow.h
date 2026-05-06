#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LSGroupTableRow.generated.h"

USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSGroupTableRow : public FTableRowBase
{
	GENERATED_BODY()

	// 그룹 이름 출력용 텍스트
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Drop")
	FText Group_Table_Text;

	// 그룹 테이블 인덱스 (도감, 정렬 등에 사용)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Drop")
	int32 Group_Table_Index = 0;

	// 드랍할 아이템 테이블의 RowName
	// 아이템 종류에 따라 prefix가 결정됨:
	// Chip_이름 → ChipTable, Weapon_이름 → WeaponTable
	// Armor_이름 → ArmorTable, Item_이름 → ItemTable
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Drop")
	FName Item_Name;

	// 가중치 (누적 가중치 알고리즘, 높을수록 선택 확률 높음)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Drop")
	int32 Group_Weight = 0;
};
