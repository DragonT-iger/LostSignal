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

	// 타이틀 Continue 활성 여부 판단용. 저장된 세이브 파일이 존재하는지.
	UFUNCTION(BlueprintPure, Category="LS/Save")
	bool HasExistingSave() const;

	// 타이틀 New 게임. 모든 진행 데이터를 초기화하고 즉시 저장한다.
	UFUNCTION(BlueprintCallable, Category="LS/Save")
	void StartNewGame();

	// 현재 레이드(파밍) 세션이 진행 중인지. 레이드에서만 신호 게이지가 시간에 따라 감소한다.
	UFUNCTION(BlueprintPure, Category="LS/Save")
	bool IsRaidSaveActive() const;

	UFUNCTION(BlueprintPure, Category="LS/Save")
	const TArray<FLSSessionItem>& GetInventory() const;

	UFUNCTION(BlueprintPure, Category="LS/Save")
	const TArray<FLSSessionItem>& GetWarehouseItems() const;

	UFUNCTION(BlueprintPure, Category="LS/Save")
	const TArray<FLSSessionItem>& GetSafeStash() const;

	UFUNCTION(BlueprintPure, Category="LS/Save")
	const TArray<FLSSessionItem>& GetChipEquipmentSlots() const;

	UFUNCTION(BlueprintPure, Category="LS/Save")
	const TArray<FLSSessionItem>& GetEquipmentSlots() const;

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
	// 무기/방어구 장비 슬롯 이동. FromArea/ToArea 중 정확히 하나가 Equipment이며, 로비 전용이다.
	// 장착(Inventory/Safe -> Equipment)/해제(Equipment -> Inventory/Safe)/교환을 타입 검증과 함께 처리한다.
	bool MoveEquipmentSlot(ELSInventorySlotArea FromArea, int32 FromIndex, ELSInventorySlotArea ToArea, int32 ToIndex);
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
	// 현재 슬롯(PIE 포함)과 기본 슬롯의 세이브 파일·디버그 json을 모두 삭제한다.
	void DeleteAllSaveFiles() const;
	void SaveDebugJson() const;
	void ResolveInterruptedRaid();
	void MigrateInventory();
	void EnsureChipEquipmentSlots();
	void EnsureEquipmentSlots();
	void ApplyStarterItems();
	void ApplyConfiguredStarterItems();
	void ApplyLowestGradeChipStarterItems();
	void AddStarterItemToArea(FName ItemRowName, int32 Amount, ELSInventorySlotArea TargetArea, const TArray<FLSChipResolvedStat>& ChipStats, const TCHAR* SourceLabel);
	int32 GetStarterTargetMaxSlotCount(ELSInventorySlotArea TargetArea) const;
	int32 GetCarryingProtocolSlotBonus(FName EnableName) const;
	TArray<FLSSessionItem>& GetMutableInventory();
	TArray<FLSSessionItem>* GetMutableStoredSlots(ELSInventorySlotArea SlotArea);
	const TArray<FLSSessionItem>* GetStoredSlots(ELSInventorySlotArea SlotArea) const;

	UPROPERTY() TObjectPtr<ULSSaveGame> SaveData;

	static const FString SlotName;
	static const FString DebugFileName;
};
