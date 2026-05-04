#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LSDropTableRow.generated.h"

USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSDropTableRow : public FTableRowBase
{
	GENERATED_BODY()

	// G_##### = GroupTable 랜덤드랍, I_/C_/W_/A_ = 고정드랍 아이템 RowName
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Drop")
	FString Item_Table_ID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Drop")
	float Drop_Rate = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Drop")
	int32 Drop_Amount = 0;
};
