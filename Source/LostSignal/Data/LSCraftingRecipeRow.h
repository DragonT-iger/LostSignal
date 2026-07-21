#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LSCraftingRecipeRow.generated.h"

UENUM(BlueprintType)
enum class ELSCraftingCategory : uint8
{
	All,
	Weapon,
	Armor,
	Chip,
	Consumable
};

USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSCraftingMaterialCost
{
	GENERATED_BODY()

	// 제작에 소모할 재료 아이템 RowName. 현재 제작 재료는 인스턴스 스탯이 없는 아이템만 허용한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Crafting")
	FName ItemRowName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Crafting", meta=(ClampMin="1"))
	int32 RequiredAmount = 1;
};

// 제작 레시피 한 줄. 결과 아이템의 표시 정보는 기존 아이템 DataTable이 단일 출처다.
USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSCraftingRecipeRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Crafting")
	FName ResultItemRowName;

	// 필요한 재료 목록. 제작 화면은 배열 길이만큼 재료 슬롯을 동적으로 생성한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Crafting", meta=(TitleProperty="ItemRowName"))
	TArray<FLSCraftingMaterialCost> Materials;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Crafting", meta=(ClampMin="0"))
	int32 GoldCost = 0;

	// 제작 목록 표시 순서. 값이 같으면 레시피 RowName 오름차순으로 정렬한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Crafting")
	int32 SortOrder = 0;
};
