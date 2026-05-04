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

	UPROPERTY(BlueprintReadOnly) FString ItemRowName;
	UPROPERTY(BlueprintReadOnly) int32 Amount = 0;
	UPROPERTY(BlueprintReadOnly) FString ItemName;
};

UCLASS()
class LOSTSIGNAL_API ULSDropSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category="LS/Drop")
	TArray<FLSDropResult> RollDropTable(int32 DropTableID);

	UFUNCTION(BlueprintCallable, Category="LS/Drop")
	FString RollGroupTable(int32 GroupID);

	UFUNCTION(BlueprintCallable, Category="LS/Drop")
	TArray<FLSDropResult> OpenRootingObject(const FString& RootingObjectRowName);

	UFUNCTION(BlueprintCallable, Category="LS/Drop")
	void TestDrop(const FString& RootingObjectRowName);

private:
	void LoadTables();
	void CacheDropTable();
	void CacheGroupTable();
	FString FindItemName(const FString& ItemRowName) const;

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

	TMap<int32, TArray<const FLSDropTableRow*>> DropTableMap;
	TMap<int32, TArray<const FLSGroupTableRow*>> GroupTableMap;
};
