#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LSDropSubsystem.generated.h"

struct FLSDropTableRow;
struct FLSGroupTableRow;

USTRUCT(BlueprintType)
struct FLSDropResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FName ItemRowName;
	UPROPERTY(BlueprintReadOnly) int32 Amount = 0;
	UPROPERTY(BlueprintReadOnly) FText ItemText;

	// 칩 드랍 시 부여되는 스탯 롤링 시드. 0 = 비칩. ToSessionItem으로 슬롯에 그대로 전달된다.
	UPROPERTY(BlueprintReadOnly) int32 StatSeed = 0;
};

UCLASS()
class LOSTSIGNAL_API ULSDropSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// DropTable prefix로 드랍 롤 (예: "Drop_Chip_Chest")
	UFUNCTION(BlueprintCallable, Category="LS/Drop")
	TArray<FLSDropResult> RollDropTable(FName DropTableName);

	// GroupTable prefix로 가중치 랜덤 아이템 1개 선택 (예: "Group_Chip_Supply")
	UFUNCTION(BlueprintCallable, Category="LS/Drop")
	FName RollGroupTable(FName GroupTableName);

	UFUNCTION(BlueprintCallable, Category="LS/Drop")
	TArray<FLSDropResult> OpenRootingObject(const FName& RootingObjectRowName);

	UFUNCTION(BlueprintCallable, Category="LS/Drop")
	void TestDrop(const FName& RootingObjectRowName);

private:
	void LoadTables();
	void CacheDropTable();
	void CacheGroupTable();
	FText FindItemText(const FName& ItemRowName) const;

#if WITH_EDITOR
	void ValidateGroupReferences();
#endif

	UPROPERTY()
	TObjectPtr<UDataTable> RootingObjectTable;
	UPROPERTY()
	TObjectPtr<UDataTable> DropTableData;
	UPROPERTY()
	TObjectPtr<UDataTable> GroupTableData;
	UPROPERTY()
	TObjectPtr<UDataTable> ChipTable;
	UPROPERTY()
	TObjectPtr<UDataTable> WeaponTable;
	UPROPERTY()
	TObjectPtr<UDataTable> ArmorTable;
	UPROPERTY()
	TObjectPtr<UDataTable> ItemTable;

	// key = DropTable RowName prefix (예: Drop_Chip_Chest)
	TMap<FName, TArray<const FLSDropTableRow*>> DropTableMap;
	// key = GroupTable RowName prefix (예: Group_Chip_Supply)
	TMap<FName, TArray<const FLSGroupTableRow*>> GroupTableMap;
};
