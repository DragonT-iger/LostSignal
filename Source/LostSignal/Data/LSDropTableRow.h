#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LSDropTableRow.generated.h"

USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSDropTableRow : public FTableRowBase
{
	GENERATED_BODY()

	// 드랍 테이블 이름 출력용 텍스트
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Drop")
	FText Drop_Table_Text;

	// 드랍 테이블 인덱스 (같은 인덱스 = 같은 그룹, 도감/정렬용)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Drop")
	int32 Drop_Table_Index = 0;

	// 참조할 GroupTable RowName prefix (예: Group_Chip_Supply)
	// GroupTable의 Row Name이 이 값으로 시작하는 항목들이 가중치 풀을 구성
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Drop")
	FName Group_Table_Name;

	// 이 드랍 항목이 발동될 확률 (0~100)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Drop")
	float Drop_Rate = 0.0f;

	// 한번에 드랍되는 아이템 개수
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Drop")
	int32 Drop_Amount = 0;
};
