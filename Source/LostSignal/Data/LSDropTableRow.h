#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LSDropTableRow.generated.h"

USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSDropTableRow : public FTableRowBase
{
	GENERATED_BODY()

	// 0=고정드랍, 1=그룹랜덤드랍
	// Legacy type hint. Item_Table_ID prefix is authoritative: G_#### = group, I_/C_/W_/A_ = item row.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Drop")
	int32 Drop_Table_Type = 0;

	// Type=0이면 아이템 RowName (예: C_0001), Type=1이면 GroupTable ID (예: 30000)
	// Examples: G_30000 for GroupTable, I_1/C_1/W_1/A_1 for item tables.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Drop")
	FString Item_Table_ID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Drop")
	float Drop_Rate = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Drop")
	int32 Drop_Amount = 0;
};
