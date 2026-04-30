#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LSGroupTableRow.generated.h"

USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSGroupTableRow : public FTableRowBase
{
	GENERATED_BODY()

	// 아이템 RowName (예: C_0001, W_0001)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Drop")
	FString Group_Item_ID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Drop")
	int32 Group_Weight = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Drop")
	FString Group_Name;
};
