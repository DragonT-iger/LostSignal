#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LSItemRow.generated.h"

USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSItemRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	FString Item_Name;

	// 3=일반, 4~9=소모품, 11~19=퀘스트, 20~=재료
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	int32 Item_Type = 3;

	// 0=보급, 1=표준, 2=정밀, 3=튜닝, 4=프로토타입, 5=마스터피스
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	int32 Item_Grade = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	int32 Item_Max = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	FString Item_Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Item")
	int32 Item_Cost = 0;
};
