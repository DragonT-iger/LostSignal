#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LSDropTableRow.generated.h"

USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSDropTableRow : public FTableRowBase
{
	GENERATED_BODY()

	// 0=고정드랍, 1=그룹랜덤드랍
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Drop")
	int32 Drop_Table_Type = 0;

	// Type=0이면 아이템 RowName (예: C_0001), Type=1이면 GroupTable ID (예: 30000)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Drop")
	FString Item_Table_ID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Drop")
	float Drop_Rate = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Drop")
	int32 Drop_Amount = 0;
};
