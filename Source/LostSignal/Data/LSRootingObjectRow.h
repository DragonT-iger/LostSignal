#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LSRootingObjectRow.generated.h"

USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSRootingObjectRow : public FTableRowBase
{
	GENERATED_BODY()

	// 오브젝트 이름 출력용 텍스트
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Drop")
	FText Loot_Object_Text;

	// 고유 인덱스 (도감, 정렬 등에 사용)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Drop")
	int32 Looting_Object_Index = 0;

	// 0=일반 오픈, 1=열쇠 오픈
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Drop")
	int32 Looting_Object_Interaction = 0;

	// 참조할 DropTable RowName prefix (예: Drop_Chip_Chest)
	// DropTable의 Row Name이 이 값으로 시작하는 항목들이 적용됨
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Drop")
	FName Drop_Table_Name;
};
