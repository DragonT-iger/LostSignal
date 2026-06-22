#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Session/LSSessionSubsystem.h"
#include "LSSaveSubsystem.generated.h"

class ULSSaveGame;

// 칩 장착 슬롯 또는 신호 게이지가 바뀌면 발행된다. (전투 스탯 재적용 등에 사용)
DECLARE_MULTICAST_DELEGATE(FLSOnChipLoadoutChanged);

UCLASS()
class LOSTSIGNAL_API ULSSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// 칩 장착/신호 게이지 변경 알림. ULSChipStatComponent 등이 구독한다.
	FLSOnChipLoadoutChanged OnChipLoadoutChanged;

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
	int32 GetMaxInventorySlotCount() const;

	UFUNCTION(BlueprintPure, Category="LS/Save")
	int32 GetMaxSafeStashSlotCount() const;

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
	// 외부 아이템(예: 룻박스 결과)을 인벤토리/금고의 특정 슬롯에 스택/배치한다. 남은 수량은 InOut 인자로 돌려준다.
	bool DropExternalItemToStoredSlot(FLSSessionItem& InOutExternalItem, ELSInventorySlotArea ToArea, int32 ToIndex);
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
	int32 GetCarryingProtocolSlotBonus(FName EnableName) const;
	TArray<FLSSessionItem>& GetMutableInventory();
	TArray<FLSSessionItem>* GetMutableStoredSlots(ELSInventorySlotArea SlotArea);
	const TArray<FLSSessionItem>* GetStoredSlots(ELSInventorySlotArea SlotArea) const;

	UPROPERTY() TObjectPtr<ULSSaveGame> SaveData;

	static const FString SlotName;
	static const FString DebugFileName;
};
