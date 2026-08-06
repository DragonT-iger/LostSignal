// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/LSProtocolTypes.h"
#include "Data/LSDropSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "Session/LSSessionSubsystem.h"
#include "TimerManager.h"
#include "UI/Combat/LSDamageNumberTypes.h"
#include "LSPlayerControllerBase.generated.h"

class ALSLootBox;
class ALSWorldDroppedItem;
class UInputMappingContext;
class ULSLobbyStorageWidget;
class ULSInventoryWidget;
class ULSQuickSlotBarWidget;
class ULSChipStationWidget;
class ULSRaidInventoryComponent;
class ULSSaveSubsystem;
class ULSHpDebugWidget;
class ULSLootDropWidget;
class ULSPlayerHUDWidget;
class ULSProtocolDebugWidget;
class ULSSettingsWidget;
struct FLSNoiseEvent;

USTRUCT()
struct FLSPendingOverflowWorldDropItem
{
	GENERATED_BODY()

	UPROPERTY(Transient, VisibleAnywhere, Category="LS/Inventory")
	FLSSessionItem Item;

	UPROPERTY(Transient, VisibleAnywhere, Category="LS/Inventory")
	FVector DropDirection = FVector::ForwardVector;

	UPROPERTY(Transient, VisibleAnywhere, Category="LS/Inventory")
	float DropDistance = 0.0f;
};

UCLASS(Abstract)
class LOSTSIGNAL_API ALSPlayerControllerBase : public APlayerController
{
	GENERATED_BODY()

public:
	ALSPlayerControllerBase();

	UFUNCTION(BlueprintPure, Category="LS/UI")
	TSubclassOf<ULSHpDebugWidget> GetDebugHpWidgetClass() const { return DebugHpWidgetClass; }

	ULSRaidInventoryComponent* GetRaidInventoryComponent() const { return RaidInventoryComponent; }
	const TArray<TObjectPtr<UInputMappingContext>>& GetDefaultMappingContexts() const { return DefaultMappingContexts; }

	UFUNCTION(BlueprintCallable, Category="LS/UI")
	void ShowLootDropWidget(const FText& LootSourceName, const TArray<FLSDropResult>& Results, ALSLootBox* SourceLootBox);

	UFUNCTION(BlueprintCallable, Category="LS/UI")
	void HideLootDropWidget();

	UFUNCTION(BlueprintCallable, Category="LS/UI")
	void ShowLobbyStorageWidget(TSubclassOf<ULSLobbyStorageWidget> LobbyStorageWidgetClass);

	UFUNCTION(BlueprintCallable, Category="LS/UI")
	void HideLobbyStorageWidget();

	UFUNCTION(BlueprintPure, Category="LS/UI")
	bool IsLobbyStorageWidgetOpen() const;

	UFUNCTION(BlueprintCallable, Category="LS/UI")
	void ShowChipStationWidget(TSubclassOf<ULSChipStationWidget> ChipStationWidgetClass);

	UFUNCTION(BlueprintCallable, Category="LS/UI")
	void HideChipStationWidget();

	UFUNCTION(BlueprintPure, Category="LS/UI")
	bool IsChipStationWidgetOpen() const;

	// 로비(폰 없음)에서 오버레이로 띄운 인벤토리/창고 위젯을 PC에 등록한다.
	// 폰에 묶여 있던 갱신/열림 판정이 폰이 없을 때 이 위젯들로 우회된다. 위젯이 스스로 생성/소멸 시 호출한다.
	void RegisterLobbyInventoryWidget(ULSInventoryWidget* InWidget);
	void UnregisterLobbyInventoryWidget(const ULSInventoryWidget* InWidget);

	// 로비에서 오버레이로 띄운 인벤토리 위젯(장비 장착칸 소유). 없으면 nullptr. 장비칸 드래그 하이라이트 등에 쓴다.
	ULSInventoryWidget* GetLobbyInventoryWidget() const { return LobbyInventoryWidgetInstance; }
	void RegisterLobbyStorageWidget(ULSLobbyStorageWidget* InWidget);
	void UnregisterLobbyStorageWidget(const ULSLobbyStorageWidget* InWidget);

	// 로비/레이드 HUD에 배치된 퀵슬롯 바를 활성 바로 등록한다(바 위젯이 생성/소멸 시 스스로 호출).
	// RefreshAllInventoryUI가 개수 갱신을 이 바에 태운다.
	void RegisterQuickSlotBar(ULSQuickSlotBarWidget* InWidget);
	void UnregisterQuickSlotBar(const ULSQuickSlotBarWidget* InWidget);

	// 폰이 있으면 폰의 인벤토리 위젯을, 없으면 등록된 로비 인벤토리 위젯을 갱신한다.
	void RefreshActiveInventoryWidget();

	// 열려 있는 모든 인벤토리 계열 패널(인벤토리/Safe/장비/창고/칩스테이션)을 authoritative 데이터에서 통째로 다시 그린다.
	// 데이터를 바꾼 어떤 경로든 이 함수 하나만 호출하면 화면이 데이터와 정합된다.
	// 소스 슬롯만 낙관적으로 비우는 부분 갱신은 금지 — 반드시 이 funnel로 전체를 다시 그린다.
	void RefreshAllInventoryUI();

	// 등록된 모든 퀵슬롯 바(인벤토리+HUD)를 다시 그린다(개수·해금 가시성 재평가).
	// 인벤토리 변경 funnel과 디버그 프로토콜 레벨 변경 양쪽에서 호출한다.
	void RefreshRegisteredQuickSlotBars();

	// 폰의 인벤토리가 열려 있거나, 등록된 로비 인벤토리 위젯이 보이면 true.
	bool IsInventoryUIOpen() const;

	// 모달 패널(인벤토리/창고/칩스테이션/루트드랍)이 하나라도 보이면 true. 매번 현재 상태를 재계산한다.
	bool IsAnyModalPanelOpen() const;

	void RefreshOpenLobbyStorageWidget();

	// 칩 스테이션이 열려 있으면 칩 리스트를 다시 그린다. 인벤토리/창고 "창"에서 칩을 옮기거나 버려
	// 원본 슬롯이 비면, 칩 스테이션 칩 리스트가 그 칸을 stale로 들고 있어 빠른 장착이 빈 슬롯을 가리키게 된다.
	// 창 주도 편집 직후 호출한다. (칩 스테이션 내부 빠른 장착 경로에서는 호출하지 않는다 — 쓸기 중 리스트 재정렬 방지.)
	void RefreshOpenChipStationWidget();

	// 소모품 시전 게이지 표시/숨김. HUD의 스킬 캐스팅 게이지(ShowSkillCastGauge)를 재사용한다.
	// 로컬 컨트롤러 + HUD 존재 시에만 동작한다.
	void ShowCastGauge(const FText& Label, float Duration);
	void HideCastGauge();

	// 소모품 회복 미리보기 표시/클리어. HUD 생존 상태 위젯의 프리뷰 체력 바로 전달한다.
	// 로컬 컨트롤러 + HUD 존재 시에만 동작한다(캐스트 게이지와 동일).
	void ShowHealthRecoveryPreview(float TargetHealth, float Duration);
	void ClearHealthRecoveryPreview();

	void RefreshLootDropWidgetForSource(ALSLootBox* SourceLootBox, const TArray<FLSDropResult>& Results);
	void SyncRaidInventoryToClient();
	void RequestRaidEntryDataForRaidStart();
	void RequestRaidResultSave(ELSRaidResult Result, const TArray<FLSSessionItem>& InventoryItems, const TArray<FLSSessionItem>& SafeItems, const TArray<FLSSessionItem>& EquipmentItems, bool bSaveInventory, bool bSaveSafeStash, bool bSaveEquipment);
	void ClearSubmittedRaidEntryData();
	bool HasSubmittedRaidEntryData() const { return bHasSubmittedRaidEntryData; }
	const TArray<FLSSessionItem>& GetSubmittedRaidLoadout() const { return SubmittedRaidLoadout; }
	const TArray<FLSSessionItem>& GetSubmittedRaidSafeItems() const { return SubmittedRaidSafeItems; }
	const TArray<FLSSessionItem>& GetSubmittedRaidEquipment() const { return SubmittedRaidEquipment; }

	UFUNCTION(BlueprintCallable, Category="LS/UI")
	bool TransferHoveredLootDropItemToInventory();

	UFUNCTION(Exec)
	void LSTestSurvivalProtocol(int32 Level);

	UFUNCTION(Exec)
	void LSTestCarryingProtocol(int32 Level);

	UFUNCTION(Exec)
	void LSTestBattleProtocol(int32 Level);

	UFUNCTION(Exec)
	void LSTestNavigationProtocol(int32 Level);

	UFUNCTION(Exec)
	void LSTestAllProtocols(int32 Survival, int32 Carrying, int32 Battle, int32 Navigation);

	UFUNCTION(Exec)
	void LSTestSkillCastGauge(float Duration);

	UFUNCTION(Exec)
	void LSClearSurvivalProtocolTest();

	UFUNCTION(Exec)
	void LSClearProtocolTest();

	// 시연용: 프로토콜 조정 패널 토글 (기본 F1, 콘솔 백업 경로 제공).
	UFUNCTION(Exec)
	void LSToggleProtocolDebug();

	void ToggleProtocolDebugWidget();

	// 프로토콜 디버그 패널이 현재 화면에 떠 있는지. 패널이 떠 있을 때만 오버라이드를 따른다.
	bool IsProtocolDebugWidgetVisible() const;

	// 레이드 중 ESC로 여는 세팅 화면(WBP_Settings) 토글. 레이드 중이 아니면 무시한다.
	void ToggleRaidSettingsWidget();

	// 세팅 화면의 BackButton으로 스스로 닫혔을 때(RemoveFromParent) 캐시를 비워 다음 ESC에서 다시 생성되게 한다.
	UFUNCTION()
	void HandleRaidSettingsClosed();

	bool HasSurvivalProtocolTestLevel() const { return HasProtocolTestLevel(ELSProtocolType::Survival); }
	int32 GetSurvivalProtocolTestLevel() const { return GetProtocolTestLevel(ELSProtocolType::Survival); }
	bool HasProtocolTestLevel(ELSProtocolType ProtocolType) const;
	int32 GetProtocolTestLevel(ELSProtocolType ProtocolType) const;
	// 현재 적용 중인 프로토콜 레벨: 패널 표시 중 오버라이드가 있으면 그 값, 없으면 신호 활성 장착 칩의 합산값.
	int32 GetEffectiveProtocolLevel(ELSProtocolType ProtocolType) const;

	// 해금된 퀵슬롯 칸 수(UI 표시·사용 게이트의 단일 출처). 디버그 패널 적재 오버라이드가 있으면 그 레벨을,
	// 없으면 세이브의 신호 활성 칩 기반 값을 쓴다. 두 소비처(HUD 바 가시성 / 레이드 키 사용)가 이 값 하나를 공유한다.
	int32 GetUnlockedQuickSlotCount() const;

	UFUNCTION(Client, Reliable)
	void ClientStartRaidSession(const TArray<FLSSessionItem>& Loadout, const TArray<FLSSessionItem>& SafeItems, const TArray<FLSSessionItem>& EquipmentItems);

	UFUNCTION(Client, Reliable)
	void ClientApplyRaidSignalGaugePercent(float Percent);

	bool TransferInventorySlotToLootDrop(ELSInventorySlotArea FromSlotArea, int32 FromSlotIndex);
	bool TransferInventorySlotToOpenContainer(ELSInventorySlotArea FromSlotArea, int32 FromSlotIndex, bool bRefreshOpenContainer = true);
	bool TransferSafeSlotToInventory(int32 SafeSlotIndex);
	bool DropInventorySlot(ELSInventorySlotArea FromArea, int32 FromIndex, ELSInventorySlotArea ToArea, int32 ToIndex);
	bool DropLootDropSlot(ALSLootBox* SourceLootBox, int32 FromLootSlotIndex, int32 ToLootSlotIndex);
	bool SortRaidInventory();
	bool TransferLootDropSlotToSession(ALSLootBox* SourceLootBox, int32 LootSlotIndex, FName ItemRowName, int32 Amount, FLSSessionItem& OutLootItem);
	bool TransferLootDropSlotToSessionSlot(ALSLootBox* SourceLootBox, int32 LootSlotIndex, FName ItemRowName, int32 Amount, ELSInventorySlotArea ToSlotArea, int32 ToSlotIndex, FLSSessionItem& OutLootItem);
	bool TransferSessionSlotToLootDropSlot(ALSLootBox* SourceLootBox, ELSInventorySlotArea FromSlotArea, int32 FromSlotIndex, int32 LootSlotIndex, const FLSDropResult& CurrentLootItem, FLSSessionItem& OutLootItem);
	bool DropSessionSlotToWorld(ELSInventorySlotArea SlotArea, int32 SlotIndex, TSubclassOf<ALSWorldDroppedItem> DroppedItemClass, FVector DropDirection);
	bool DropOverflowInventorySlotsToWorld();
	bool ResolveDropDirectionFromSlatePosition(FVector2D SlatePosition, FVector& OutDropDirection) const;
	void NotifyNoiseForHUD(const FLSNoiseEvent& NoiseEvent);
	void ShowDamageNumber(const FLSDamageNumberPayload& Payload);
	float GetSoundIndicatorDetectionRadiusCm() const;

protected:
	UPROPERTY(EditAnywhere, Category="LS/Input")
	TArray<TObjectPtr<UInputMappingContext>> DefaultMappingContexts;

	UPROPERTY(EditDefaultsOnly, Category="LS/UI")
	TSubclassOf<ULSHpDebugWidget> DebugHpWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="LS/UI")
	TSubclassOf<ULSLootDropWidget> LootDropWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="LS/UI")
	TSubclassOf<ULSPlayerHUDWidget> PlayerHUDWidgetClass;

	// 레이드 중 ESC로 여는 세팅 화면. 타이틀/로비와 동일한 WBP_Settings를 재사용한다.
	UPROPERTY(EditDefaultsOnly, Category="LS/UI")
	TSubclassOf<ULSSettingsWidget> RaidSettingsWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI|Noise", meta=(ClampMin="0.0"))
	float SoundIndicatorDetectionRadiusMeters = 10.0f;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSHpDebugWidget> DebugHpWidgetInstance;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSLootDropWidget> LootDropWidgetInstance;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSPlayerHUDWidget> PlayerHUDWidgetInstance;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSLobbyStorageWidget> LobbyStorageWidgetInstance;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSChipStationWidget> ChipStationWidgetInstance;

	// 로비 오버레이에 배치돼 PC에 등록된 인벤토리 위젯(폰 없는 로비용). 폰이 있으면 사용하지 않는다.
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSInventoryWidget> LobbyInventoryWidgetInstance;

	// 현재 등록된 퀵슬롯 바들. 인벤토리 패널의 바와 HUD의 바가 동시에 떠 있을 수 있으므로 배열로 모두 들고 함께 갱신한다.
	// 바 위젯이 스스로 등록/해제하며, 약참조라 미해제 시에도 dangling이 없다.
	TArray<TWeakObjectPtr<ULSQuickSlotBarWidget>> RegisteredQuickSlotBars;

	// 시연용 프로토콜 조정 패널 인스턴스.
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="LS/Debug")
	TObjectPtr<ULSProtocolDebugWidget> ProtocolDebugWidgetInstance;

	// 레이드 중 ESC로 여는 세팅 화면 인스턴스.
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSSettingsWidget> RaidSettingsWidgetInstance;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="LS/Input")
	bool bDefaultMappingContextsApplied = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Inventory")
	TObjectPtr<ULSRaidInventoryComponent> RaidInventoryComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Inventory", meta=(ClampMin="0"))
	float DroppedItemForwardDistance = 100.0f;

	// 수동 월드 드랍에서 최고 이동속도일 때 캐릭터 이동 방향으로 더하는 거리. 정지 중에는 적용하지 않는다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Inventory", meta=(ClampMin="0"))
	float ManualDropMaxSpeedDistanceBonus = 200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Inventory", meta=(ClampMin="1"))
	int32 OverflowDropItemsPerRing = 6;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Inventory", meta=(ClampMin="0"))
	float OverflowDropRingSpacing = 75.0f;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="LS/Inventory")
	bool bHasSubmittedRaidEntryData = false;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="LS/Inventory")
	TArray<FLSSessionItem> SubmittedRaidLoadout;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="LS/Inventory")
	TArray<FLSSessionItem> SubmittedRaidSafeItems;

	// 레이드 입장 시점 무기/방어구 장착 5칸. 인덱스=슬롯 타입이므로 Normalize 금지.
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="LS/Inventory")
	TArray<FLSSessionItem> SubmittedRaidEquipment;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="LS/Debug", meta=(ClampMin="-1"))
	int32 SurvivalProtocolTestLevel = -1;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="LS/Debug", meta=(ClampMin="-1"))
	int32 CarryingProtocolTestLevel = -1;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="LS/Debug", meta=(ClampMin="-1"))
	int32 BattleProtocolTestLevel = -1;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="LS/Debug", meta=(ClampMin="-1"))
	int32 NavigationProtocolTestLevel = -1;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void AcknowledgePossession(APawn* InPawn) override;

private:
	UFUNCTION(Server, Reliable)
	void ServerDropSessionSlotToWorld(ELSInventorySlotArea SlotArea, int32 SlotIndex, TSubclassOf<ALSWorldDroppedItem> DroppedItemClass, FVector_NetQuantizeNormal DropDirection);

	UFUNCTION(Server, Reliable)
	void ServerDropOverflowInventorySlotsToWorld();

	UFUNCTION(Server, Reliable)
	void ServerTransferLootDropSlotToSession(ALSLootBox* SourceLootBox, int32 LootSlotIndex);

	UFUNCTION(Server, Reliable)
	void ServerTransferLootDropSlotToSessionSlot(ALSLootBox* SourceLootBox, int32 LootSlotIndex, ELSInventorySlotArea ToSlotArea, int32 ToSlotIndex);

	UFUNCTION(Server, Reliable)
	void ServerTransferSessionSlotToLootDropSlot(ALSLootBox* SourceLootBox, ELSInventorySlotArea FromSlotArea, int32 FromSlotIndex, int32 LootSlotIndex);

	UFUNCTION(Server, Reliable)
	void ServerDropInventorySlot(ELSInventorySlotArea FromArea, int32 FromIndex, ELSInventorySlotArea ToArea, int32 ToIndex);

	UFUNCTION(Server, Reliable)
	void ServerTransferSafeSlotToInventory(int32 SafeSlotIndex);

	UFUNCTION(Server, Reliable)
	void ServerDropLootDropSlot(ALSLootBox* SourceLootBox, int32 FromLootSlotIndex, int32 ToLootSlotIndex);

	UFUNCTION(Server, Reliable)
	void ServerSortRaidInventory();

	UFUNCTION(Client, Reliable)
	void ClientRequestRaidEntryData();

	UFUNCTION(Server, Reliable)
	void ServerSubmitRaidEntryData(const TArray<FLSSessionItem>& Loadout, const TArray<FLSSessionItem>& SafeItems, const TArray<FLSSessionItem>& EquipmentItems);

	UFUNCTION(Client, Reliable)
	void ClientApplyRaidResult(ELSRaidResult Result, const TArray<FLSSessionItem>& InventoryItems, const TArray<FLSSessionItem>& SafeItems, const TArray<FLSSessionItem>& EquipmentItems, bool bSaveInventory, bool bSaveSafeStash, bool bSaveEquipment);

	UFUNCTION(Server, Reliable)
	void ServerConfirmRaidResultSaved();

	UFUNCTION(Client, Reliable)
	void ClientShowLootDropWidget(const FText& LootSourceName, const TArray<FLSDropResult>& Results, ALSLootBox* SourceLootBox);

	UFUNCTION(Client, Reliable)
	void ClientHideLootDropWidget();

	UFUNCTION(Client, Reliable)
	void ClientShowLobbyStorageWidget(TSubclassOf<ULSLobbyStorageWidget> LobbyStorageWidgetClass);

	UFUNCTION(Client, Reliable)
	void ClientHideLobbyStorageWidget();

	UFUNCTION(Client, Reliable)
	void ClientShowChipStationWidget(TSubclassOf<ULSChipStationWidget> ChipStationWidgetClass);

	UFUNCTION(Client, Reliable)
	void ClientHideChipStationWidget();

	UFUNCTION(Client, Reliable)
	void ClientSyncRaidSessionAndLoot(ALSLootBox* SourceLootBox, const TArray<FLSSessionItem>& InventoryItems, const TArray<FLSSessionItem>& SafeItems, const TArray<FLSSessionItem>& EquipmentItems, const TArray<FLSDropResult>& LootResults);

	// 로비 파밍용: 레이드 세션이 없을 때 룻박스/세이브 기반 인벤토리 UI를 갱신한다.
	UFUNCTION(Client, Reliable)
	void ClientRefreshLobbyLoot(ALSLootBox* SourceLootBox, const TArray<FLSDropResult>& LootResults);

	UFUNCTION(Client, Reliable)
	void ClientReceiveNoiseForHUD(FVector_NetQuantize NoiseLocation, float RadiusCm, FGameplayTag NoiseTag, AActor* NoiseInstigator);

	UFUNCTION(Client, Unreliable)
	void ClientShowDamageNumber(const FLSDamageNumberPayload& Payload);

	void ShowLootDropWidgetLocal(const FText& LootSourceName, const TArray<FLSDropResult>& Results, ALSLootBox* SourceLootBox);
	void HideLootDropWidgetLocal();
	void ShowLobbyStorageWidgetLocal(TSubclassOf<ULSLobbyStorageWidget> LobbyStorageWidgetClass);
	void HideLobbyStorageWidgetLocal();
	void ShowChipStationWidgetLocal(TSubclassOf<ULSChipStationWidget> ChipStationWidgetClass);
	void HideChipStationWidgetLocal();
	void CreatePlayerHUDWidgetLocal();
	void InitializeRaidInventoryFromSessionSubsystem();
	void SubmitLocalRaidEntryData();
	void StoreSubmittedRaidEntryData(const TArray<FLSSessionItem>& Loadout, const TArray<FLSSessionItem>& SafeItems, const TArray<FLSSessionItem>& EquipmentItems);
	void ApplyRaidResultToLocalSave(ELSRaidResult Result, const TArray<FLSSessionItem>& InventoryItems, const TArray<FLSSessionItem>& SafeItems, const TArray<FLSSessionItem>& EquipmentItems, bool bSaveInventory, bool bSaveSafeStash, bool bSaveEquipment);
	// 서버 확정 경로에서 장착칸이 관여한 변경이 성공했을 때 소유 폰의 장비 전투 스탯을 재적용한다.
	void RefreshEquipmentStatsIfEquipmentTouched(ELSInventorySlotArea FromArea, ELSInventorySlotArea ToArea);
	void SyncRaidSessionAndLootFromServer(ALSLootBox* SourceLootBox);
	// 로비 파밍용: 레이드 세션이 비활성(=로비)이면 룻박스 아이템을 세이브에 직접 적재한다.
	bool IsLobbyLootMode() const;
	ULSSaveSubsystem* ResolveSaveSubsystem() const;
	void HandleNoiseForHUD(FVector NoiseLocation, float RadiusCm, FGameplayTag NoiseTag, AActor* NoiseInstigator);
	void ShowDamageNumberLocal(const FLSDamageNumberPayload& Payload);
	void SetProtocolTestLevel(ELSProtocolType ProtocolType, int32 Level);
	void RefreshProtocolTestTargets();
	void RefreshAllInventoryUIImmediate();
	void QueueInventoryUIRefreshAfterDrag();
	void FlushQueuedInventoryUIRefresh();
	bool DropSessionSlotToWorldInternal(ELSInventorySlotArea SlotArea, int32 SlotIndex, TSubclassOf<ALSWorldDroppedItem> DroppedItemClass, FVector DropDirection);
	bool DropOverflowInventorySlotsToWorldInternal();
	void QueueOverflowWorldDropItems(TArray<FLSSessionItem>&& ExtractedItems);
	FVector BuildOverflowWorldDropDirection(int32 ItemIndex, int32 ItemCount) const;
	float BuildOverflowWorldDropDistance(int32 ItemIndex) const;
	void BuildManualWorldDropTrajectory(FVector RequestedDropDirection, FVector& OutDropDirection, float& OutDropDistance) const;
	// 현재 수평 속력 ÷ 걸음새 무관 최고 이동속도(0~1). 수동 드랍 거리 보너스의 계수.
	float ResolveManualDropSpeedRatio() const;
	void FlushPendingOverflowWorldDrops();
	void SchedulePendingOverflowWorldDropRetry();
	bool SpawnDroppedItemToWorld(const FLSSessionItem& SlotItem, TSubclassOf<ALSWorldDroppedItem> DroppedItemClass, FVector DropDirection, float DropDistance = -1.0f, bool bRequireGround = false);
	bool ResolveServerDroppedItemTransform(FTransform& OutDropTransform, FVector DropDirection, float DropDistance, bool bRequireGround) const;
	bool ResolveServerDroppedItemGroundLocation(FVector& OutGroundLocation, FVector DropDirection, float DropDistance, bool bRequireGround) const;
	bool TransferLootDropSlotToSessionInternal(ALSLootBox* SourceLootBox, int32 LootSlotIndex, FLSSessionItem& OutLootItem);
	bool TransferLootDropSlotToSessionSlotInternal(ALSLootBox* SourceLootBox, int32 LootSlotIndex, ELSInventorySlotArea ToSlotArea, int32 ToSlotIndex, FLSSessionItem& OutLootItem);
	bool TransferSessionSlotToLootDropSlotInternal(ALSLootBox* SourceLootBox, ELSInventorySlotArea FromSlotArea, int32 FromSlotIndex, int32 LootSlotIndex, FLSSessionItem& OutLootItem);
	bool DropLootDropSlotInternal(ALSLootBox* SourceLootBox, int32 FromLootSlotIndex, int32 ToLootSlotIndex);

	bool bInventoryUIRefreshScheduled = false;

	UPROPERTY(Transient, VisibleAnywhere, Category="LS/Inventory")
	TArray<FLSPendingOverflowWorldDropItem> PendingOverflowWorldDropItems;
	FTimerHandle PendingOverflowWorldDropRetryTimerHandle;
};
