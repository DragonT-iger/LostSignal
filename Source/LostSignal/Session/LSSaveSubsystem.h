#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Session/LSSessionSubsystem.h"
#include "LSSaveSubsystem.generated.h"

class ULSSaveGame;

UCLASS()
class LOSTSIGNAL_API ULSSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category="LS/Save")
	void AddToInventory(const TArray<FLSSessionItem>& Items);

	bool TryAddToInventory(FName ItemRowName, int32 Amount, const TArray<FLSChipResolvedStat>& ChipStats, FLSSessionItem& OutRemainingItem);

	UFUNCTION(BlueprintCallable, Category="LS/Save")
	void ReplaceInventory(const TArray<FLSSessionItem>& Items);

	UFUNCTION(BlueprintCallable, Category="LS/Save")
	void ReplaceWarehouseItems(const TArray<FLSSessionItem>& Items);

	UFUNCTION(BlueprintCallable, Category="LS/Save")
	void ReplaceSafeStash(const TArray<FLSSessionItem>& Items);

	UFUNCTION(BlueprintCallable, Category="LS/Save")
	void SortInventory();

	UFUNCTION(BlueprintCallable, Category="LS/Save")
	void SortWarehouse();

	void BeginRaidSave(const TArray<FLSSessionItem>& Loadout);
	void UpdateRaidConsumedItems(const TArray<FLSSessionItem>& ConsumedItems);
	void ClearRaidSave();

	UFUNCTION(BlueprintPure, Category="LS/Save")
	const TArray<FLSSessionItem>& GetInventory() const;

	UFUNCTION(BlueprintPure, Category="LS/Save")
	const TArray<FLSSessionItem>& GetWarehouseItems() const;

	UFUNCTION(BlueprintPure, Category="LS/Save")
	const TArray<FLSSessionItem>& GetSafeStash() const;

	UFUNCTION(BlueprintPure, Category="LS/Save")
	const TArray<FLSSessionItem>& GetChipEquipmentSlots() const;

	UFUNCTION(BlueprintPure, Category="LS/Save")
	float GetChipSignalGaugePercent() const;

	UFUNCTION(BlueprintCallable, Category="LS/Save")
	void SetChipSignalGaugePercent(float Percent);

	bool EquipChipFromStoredSlot(ELSInventorySlotArea SourceArea, int32 SourceIndex, int32 EquipmentIndex);
	bool DropChipEquipmentSlot(int32 FromEquipmentIndex, int32 ToEquipmentIndex);
	bool UnequipChipToWarehouse(int32 EquipmentIndex);
	bool DropStoredSlot(ELSInventorySlotArea FromArea, int32 FromIndex, ELSInventorySlotArea ToArea, int32 ToIndex);
	bool TransferStoredSlotToArea(ELSInventorySlotArea FromArea, int32 FromIndex, ELSInventorySlotArea ToArea);
	bool TransferAllInventoryToWarehouse(int32 WarehouseMaxSlotCount, bool& bOutStoppedBecauseFull);
	bool GetStoredSlotItem(ELSInventorySlotArea SlotArea, int32 SlotIndex, FLSSessionItem& OutItem) const;
	bool ClearStoredSlot(ELSInventorySlotArea SlotArea, int32 SlotIndex);
	bool ReplaceStoredSlotItem(ELSInventorySlotArea SlotArea, int32 SlotIndex, const FLSSessionItem& NewItem, FLSSessionItem& OutPreviousItem);

private:
	FString GetResolvedSlotName() const;
	FString GetResolvedDebugFileName() const;
	void Load();
	void Save();
	void SaveDebugJson() const;
	void ResolveInterruptedRaid();
	void MigrateInventory();
	void EnsureChipEquipmentSlots();
	TArray<FLSSessionItem>& GetMutableInventory();
	TArray<FLSSessionItem>* GetMutableStoredSlots(ELSInventorySlotArea SlotArea);
	const TArray<FLSSessionItem>* GetStoredSlots(ELSInventorySlotArea SlotArea) const;

	UPROPERTY() TObjectPtr<ULSSaveGame> SaveData;

	static const FString SlotName;
	static const FString DebugFileName;
};
