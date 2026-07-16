#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LSStoreStockRow.generated.h"

// 자판기(에이베리 보급소 상점) 판매 목록 한 줄. DT_StoreStock의 Row 구조체.
// 아이템의 이름/설명/가격(Item_Cost)은 각 아이템 테이블(DT_Item 등)이 단일 출처이며 여기에 중복 저장하지 않는다.
// 카테고리(장비/소모품/칩/재료)도 Row Name 접두사와 Item_Type에서 자동 분류하므로 별도 필드를 두지 않는다.
USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSStoreStockRow : public FTableRowBase
{
	GENERATED_BODY()

	// 판매할 아이템의 RowName. 접두사로 소속 테이블이 결정된다.
	// (Chip_→ChipTable, Weapon_→WeaponTable, Armor_→ArmorTable, Item_→ItemTable)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Store")
	FName Item_Name;

	// 새로고침 1회당 재고 수. 아직 재고/새로고침 미구현이라 지금은 사용하지 않는다(추후 2차 구현).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Store", meta=(ClampMin="1"))
	int32 Stock_Max = 1;
};
