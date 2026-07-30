#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Session/LSSessionSubsystem.h"
#include "LSSaveSubsystem.generated.h"

class ULSSaveGame;
struct FLSSkillLoadout;
struct FLSCraftingRecipeRow;

// 칩 장착 슬롯 또는 신호 게이지가 바뀌면 발행된다. (전투 스탯 재적용 등에 사용)
DECLARE_MULTICAST_DELEGATE(FLSOnChipLoadoutChanged);

// 무기/방어구 장착 슬롯이 바뀌면 발행된다. (장비 전투 스탯 재적용에 사용)
DECLARE_MULTICAST_DELEGATE(FLSOnEquipmentChanged);

// 스킬 선택 슬롯이 바뀌면 발행된다. (로비 스킬 UI 갱신에 사용)
DECLARE_MULTICAST_DELEGATE(FLSOnSkillLoadoutChanged);

// 보유 골드가 바뀌면 발행된다. (상점 골드 표시 갱신에 사용)
DECLARE_MULTICAST_DELEGATE(FLSOnGoldChanged);

// 퀵슬롯 등록이 바뀌면 발행된다. (퀵슬롯 바 UI 아이콘 갱신에 사용)
DECLARE_MULTICAST_DELEGATE(FLSOnQuickSlotsChanged);

UCLASS()
class LOSTSIGNAL_API ULSSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// 칩 장착/신호 게이지 변경 알림. ULSChipStatComponent 등이 구독한다.
	FLSOnChipLoadoutChanged OnChipLoadoutChanged;

	// 무기/방어구 장착 변경 알림. ULSEquipmentStatComponent가 구독한다.
	FLSOnEquipmentChanged OnEquipmentChanged;

	// 스킬 선택 슬롯 변경 알림. 로비 스킬 로드아웃 UI가 구독한다.
	FLSOnSkillLoadoutChanged OnSkillLoadoutChanged;

	// 골드 변경 알림. 상점 UI 등이 구독한다.
	FLSOnGoldChanged OnGoldChanged;

	// 퀵슬롯 등록 변경 알림. 퀵슬롯 바 위젯이 구독한다.
	FLSOnQuickSlotsChanged OnQuickSlotsChanged;

	UFUNCTION(BlueprintPure, Category="LS/Save")
	int32 GetGold() const;

	// 골드를 지급한다. Amount가 0 이하이면 무시. 성공 시 저장 후 OnGoldChanged 발행.
	UFUNCTION(BlueprintCallable, Category="LS/Save")
	void AddGold(int32 Amount);

	// 보유 골드가 충분하면 차감한다. 부족하거나 Amount가 0 이하이면 false. 성공 시 저장 후 OnGoldChanged 발행.
	UFUNCTION(BlueprintCallable, Category="LS/Save")
	bool TrySpendGold(int32 Amount);

	// 제작 UI 기준 보유량. 보호 슬롯은 제외하고 인벤토리와 창고를 합산한다.
	int32 GetCraftingOwnedItemCount(FName ItemRowName) const;

	// 재료/골드/결과물 공간을 모두 반영한 연속 제작 가능 횟수.
	int32 GetCraftableCount(const FLSCraftingRecipeRow& Recipe) const;

	// 제작 1회를 원자적으로 처리한다. 인벤토리 재료를 먼저 쓰고 결과물도 인벤토리에 우선 넣는다.
	// 인벤토리에 못 넣으면 창고로 폴백하며, 양쪽 모두 불가능하면 저장 상태를 전혀 바꾸지 않는다.
	bool TryCraft(const FLSCraftingRecipeRow& Recipe, bool& bOutStoredInWarehouse);

	UFUNCTION(BlueprintCallable, Category="LS/Save")
	void AddToInventory(const TArray<FLSSessionItem>& Items);

	bool TryAddToInventory(FName ItemRowName, int32 Amount, const TArray<FLSChipResolvedStat>& ChipStats, FLSSessionItem& OutRemainingItem);

	UFUNCTION(BlueprintCallable, Category="LS/Save")
	void ReplaceInventory(const TArray<FLSSessionItem>& Items);

	UFUNCTION(BlueprintCallable, Category="LS/Save")
	void ReplaceWarehouseItems(const TArray<FLSSessionItem>& Items);

	UFUNCTION(BlueprintCallable, Category="LS/Save")
	void ReplaceSafeStash(const TArray<FLSSessionItem>& Items);

	// 레이드 결과(탈출/사망)의 최종 장착 상태를 세이브에 반영한다.
	// 장비 배열은 인덱스=슬롯타입 불변식이므로 Normalize 금지, SetNum(5) 패딩만 한다.
	UFUNCTION(BlueprintCallable, Category="LS/Save")
	void ReplaceEquipmentSlots(const TArray<FLSSessionItem>& Items);

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

	// CharacterID 캐릭터의 스킬 선택 슬롯 3칸(인덱스 = Skill1/2/3, 값 = Skill_ID, 0 = 빈 칸).
	UFUNCTION(BlueprintPure, Category="LS/Save")
	const TArray<int32>& GetEquippedSkillIDs(int32 CharacterID) const;

	// CharacterID 캐릭터의 SlotIndex 칸에 SkillID를 장착한다. 같은 SkillID가 다른 칸에 이미 있으면 그 칸을 비워 중복을 막는다(이동).
	// 범위 밖이거나 실패하면 false. 성공 시 저장 후 OnSkillLoadoutChanged 발행.
	UFUNCTION(BlueprintCallable, Category="LS/Save")
	bool SetEquippedSkillSlot(int32 CharacterID, int32 SlotIndex, int32 SkillID);

	// CharacterID 캐릭터의 SlotIndex 칸을 비운다. 성공 시 저장 후 OnSkillLoadoutChanged 발행.
	UFUNCTION(BlueprintCallable, Category="LS/Save")
	bool ClearEquippedSkillSlot(int32 CharacterID, int32 SlotIndex);

	// CharacterID 캐릭터의 로드아웃을 아직 초기화한 적 없으면(최초 진입/새 게임) DefaultSkillIDs로 3칸을 1회 시딩한다.
	// 이후엔 사용자가 슬롯을 다 비워도 다시 채우지 않는다. 시딩했으면 true(저장 후 OnSkillLoadoutChanged 발행).
	bool TrySeedDefaultSkillLoadout(int32 CharacterID, const TArray<int32>& DefaultSkillIDs);

	// 퀵슬롯 등록 6칸(인덱스 = 슬롯 번호, 값 = 소모품 RowName, NAME_None = 빈 칸).
	UFUNCTION(BlueprintPure, Category="LS/Save")
	const TArray<FName>& GetQuickSlots() const;

	// 적재 프로토콜로 해금된 퀵슬롯 칸 수(0~QuickSlotCount). 인벤토리 슬롯 용량과 동일 메커니즘
	// (Carrying 프로토콜 + DT_Protocol의 "Quick" UI_Slot 행). 신호 활성 장착 칩 기반(current/previous).
	UFUNCTION(BlueprintPure, Category="LS/Save")
	int32 GetUnlockedQuickSlotCount() const;

	// 주어진 적재 프로토콜 레벨로 해금 퀵슬롯 칸 수를 계산한다(디버그 패널 오버라이드 반영 경로).
	// 강제 레벨은 보호 레벨이 무의미하므로 current==previous로 다룬다(스킬 바 디버그 경로와 동일).
	int32 GetUnlockedQuickSlotCountForCarryingLevel(int32 CarryingLevel) const;

	// SlotIndex 칸에 소모품 ItemRowName을 등록한다. 소모품(Item_Type 4~9)이 아니거나 인덱스가 범위 밖이면 false.
	// 같은 아이템이 다른 칸에 이미 있으면 그 칸을 비운다(이동). 성공 시 저장 후 OnQuickSlotsChanged 발행.
	UFUNCTION(BlueprintCallable, Category="LS/Save")
	bool SetQuickSlot(int32 SlotIndex, FName ItemRowName);

	// SlotIndex 칸을 비운다. 성공 시 저장 후 OnQuickSlotsChanged 발행.
	UFUNCTION(BlueprintCallable, Category="LS/Save")
	bool ClearQuickSlot(int32 SlotIndex);

	UFUNCTION(BlueprintPure, Category="LS/Save")
	int32 GetMaxInventorySlotCount() const;

	UFUNCTION(BlueprintPure, Category="LS/Save")
	int32 GetMaxSafeStashSlotCount() const;

	// 창고 최대 슬롯 수. ULSSaveSettings가 단일 출처이며 창고 UI/자판기가 함께 읽는다.
	UFUNCTION(BlueprintPure, Category="LS/Save")
	int32 GetMaxWarehouseSlotCount() const;

	UFUNCTION(BlueprintPure, Category="LS/Save")
	float GetChipSignalGaugePercent() const;

	UFUNCTION(BlueprintCallable, Category="LS/Save")
	void SetChipSignalGaugePercent(float Percent);

	bool EquipChipFromStoredSlot(ELSInventorySlotArea SourceArea, int32 SourceIndex, int32 EquipmentIndex);
	bool DropChipEquipmentSlot(int32 FromEquipmentIndex, int32 ToEquipmentIndex);
	bool UnequipChipToWarehouse(int32 EquipmentIndex);
	// EquipmentIndex의 칩을 해제해 일반 인벤토리에 넣는다. PreferredSlotIndex가 유효한 빈 칸이면 그 칸,
	// 아니면 첫 빈 칸. 대상 칸은 "해제 후 예상 최대 슬롯 수"(적재 프로토콜 축소 반영) 미만이어야 한다.
	// 넣을 자리가 없으면 아무것도 바꾸지 않고 false를 돌려준다(호출자가 창고 폴백 판단).
	bool UnequipChipToInventory(int32 EquipmentIndex, int32 PreferredSlotIndex, int32& OutPlacedSlotIndex);
	// EquipmentIndex의 칩을 해제하면 적재(Carrying) 프로토콜이 줄어 인벤토리 최대 용량이 축소되고,
	// 그 결과 초과분이 월드로 드롭(=손실)되는지 여부. 해제 전에 호출해 해제 자체를 막는 데 쓴다.
	bool WouldUnequipChipDropInventoryItems(int32 EquipmentIndex) const;
	// EquipmentIndex의 장착 칩을 (SourceArea/SourceIndex) 저장 칩과 교체(스왑)하면 적재 프로토콜이 줄어
	// 인벤토리 초과분이 월드로 드롭(=손실)되는지 여부. 스왑 전에 호출해 스왑 자체를 막는 데 쓴다.
	bool WouldSwapChipDropInventoryItems(ELSInventorySlotArea SourceArea, int32 SourceIndex, int32 EquipmentIndex) const;
	// 무기/방어구 장비 슬롯 이동. FromArea/ToArea 중 정확히 하나가 Equipment이며, 로비 전용이다.
	// 장착(Inventory/Safe -> Equipment)/해제(Equipment -> Inventory/Safe)/교환을 타입 검증과 함께 처리한다.
	bool MoveEquipmentSlot(ELSInventorySlotArea FromArea, int32 FromIndex, ELSInventorySlotArea ToArea, int32 ToIndex);
	bool DropStoredSlot(ELSInventorySlotArea FromArea, int32 FromIndex, ELSInventorySlotArea ToArea, int32 ToIndex);
	bool TransferStoredSlotToArea(ELSInventorySlotArea FromArea, int32 FromIndex, ELSInventorySlotArea ToArea);
	bool TransferAllInventoryToWarehouse(bool& bOutStoppedBecauseFull);
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
	// 골드 필드가 없던 세이브(또는 새 데이터)에 기본 골드를 1회 지급한다.
	void EnsureGoldInitialized();
	void EnsureChipEquipmentSlots();
	void EnsureEquipmentSlots();
	// 퀵슬롯 배열을 QuickSlotCount(6)칸으로 보정한다(부족하면 NAME_None으로 채우고 초과분은 잘라낸다).
	void EnsureQuickSlots();
	// CharacterID 로드아웃 항목을 없으면 만들고 SkillIDs를 3칸으로 보정해 참조로 돌려준다. SaveData 유효성은 호출부가 보장.
	FLSSkillLoadout& EnsureSkillLoadout(int32 CharacterID);
	void ApplyStarterItems();
	void ApplyConfiguredStarterItems();
	void ApplyLowestGradeChipStarterItems();
	void AddStarterItemToArea(FName ItemRowName, int32 Amount, ELSInventorySlotArea TargetArea, const TArray<FLSChipResolvedStat>& ChipStats, const TCHAR* SourceLabel);
	int32 GetStarterTargetMaxSlotCount(ELSInventorySlotArea TargetArea) const;
	bool HasWarehouseOverflow() const;
	int32 GetCarryingProtocolSlotBonus(FName EnableName) const;
	// 주어진 칩 장착 배열 기준으로 적재 프로토콜 슬롯 보너스를 계산한다(현재 저장 상태가 아니라 가정 배열도 넣을 수 있음).
	int32 ComputeCarryingProtocolSlotBonus(const TArray<FLSSessionItem>& EquipmentSlots, FName EnableName) const;
	// 가정 장착 배열로 인벤토리 초과분이 월드 드롭되는지 예측한다(해제/스왑 차단 판정의 공용 코어).
	bool WouldChipEquipmentDropInventoryItems(const TArray<FLSSessionItem>& HypotheticalSlots) const;
	// 가정 장착 배열 기준 예상 최대 인벤토리 슬롯 수. 실제 반영 후 GetMaxInventorySlotCount()와 동일값.
	int32 ComputePredictedMaxInventorySlotCount(const TArray<FLSSessionItem>& HypotheticalSlots) const;
	// [0, MaxSlotCount) 범위에서 선호 슬롯 우선, 아니면 첫 빈 인벤토리 인덱스. 배열 Num() 밖 인덱스는 빈 칸 취급.
	int32 FindEmptyInventorySlotForUnequip(int32 PreferredSlotIndex, int32 MaxSlotCount) const;
	// 칩 해제 공통 검증(SaveData/인덱스/채워짐/Chip_ 접두). 실패 시 경고 후 nullptr.
	FLSSessionItem* ResolveFilledChipEquipmentSlot(int32 EquipmentIndex, const TCHAR* ContextLabel);
	TArray<FLSSessionItem>& GetMutableInventory();
	TArray<FLSSessionItem>* GetMutableStoredSlots(ELSInventorySlotArea SlotArea);
	const TArray<FLSSessionItem>* GetStoredSlots(ELSInventorySlotArea SlotArea) const;

	UPROPERTY() TObjectPtr<ULSSaveGame> SaveData;

	static const FString SlotName;
	static const FString DebugFileName;
};
