#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LSRootingObjectRow.generated.h"

USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSRootingObjectRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Drop")
	FString Rooting_Object_Name;

	// 0=칩상자, 1=무기상자, 2=방어구상자, 3=금고, 4=소모품상자,
	// 5=퀘스트상자, 7=몬스터, 8=재료상자, 9=복합상자
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Drop")
	int32 Rooting_Object_Type = 0;

	// 0=일반오픈, 1=열쇠오픈
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Drop")
	int32 Rooting_Object_Interaction = 0;

	// 참조할 DropTable 그룹 ID (D_20000_x 에서 20000)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Drop")
	int32 Drop_Table_ID = 0;
};
