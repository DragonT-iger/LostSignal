#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LSProtocolUnlockRow.generated.h"

USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSProtocolUnlockRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Protocol", meta=(ClampMin="0"))
	int32 Protocol_Required_Level = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Protocol")
	FName Protocol_Enable_Type;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Protocol")
	FName Protocol_Enable_Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Protocol")
	int32 Protocol_Enable_Value = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Protocol", meta=(ClampMin="0"))
	int32 Protocol_Protected_Level = 0;
};
