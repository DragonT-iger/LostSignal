// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/LSProtocolTypes.h"
#include "Data/LSDropSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "Session/LSSessionSubsystem.h"
#include "UI/Combat/LSDamageNumberTypes.h"
#include "LSPlayerControllerBase.generated.h"

class ALSLootBox;
class ALSWorldDroppedItem;
class UInputMappingContext;
class ULSLobbyStorageWidget;
class ULSInventoryWidget;
class ULSChipStationWidget;
class ULSBackgroundBlurWidget;
class ULSRaidInventoryComponent;
class ULSSaveSubsystem;
class ULSHpDebugWidget;
class ULSLootDropWidget;
class ULSPlayerHUDWidget;
class ULSProtocolDebugWidget;
class ULSSettingsWidget;
struct FLSNoiseEvent;

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
	void RegisterLobbyStorageWidget(ULSLobbyStorageWidget* InWidget);
	void UnregisterLobbyStorageWidget(const ULSLobbyStorageWidget* InWidget);

	// 폰이 있으면 폰의 인벤토리 위젯을, 없으면 등록된 로비 인벤토리 위젯을 갱신한다.
	void RefreshActiveInventoryWidget();
	// 폰의 인벤토리가 열려 있거나, 등록된 로비 인벤토리 위젯이 보이면 true.
	bool IsInventoryUIOpen() const;

	// 모달 패널(인벤토리/창고/칩스테이션/루트드랍) 표시 상태에 맞춰 공유 블러를 켜고 끈다.
	// 어느 패널이든 show/hide 직후 호출하면 되며, 매번 현재 상태를 재계산하므로 중복 호출에 안전하다.
	void UpdateBackgroundBlurVisibility();

	// 모달 패널(인벤토리/창고/칩스테이션/루트드랍)이 하나라도 보이면 true. 매번 현재 상태를 재계산한다.
	bool IsAnyModalPanelOpen() const;

	int32 GetOpenLobbyStorageMaxSlotCount() const;
	void RefreshOpenLobbyStorageWidget();

	// 칩 스테이션이 열려 있으면 칩 리스트를 다시 그린다. 인벤토리/창고 "창"에서 칩을 옮기거나 버려
	// 원본 슬롯이 비면, 칩 스테이션 칩 리스트가 그 칸을 stale로 들고 있어 빠른 장착이 빈 슬롯을 가리키게 된다.
	// 창 주도 편집 직후 호출한다. (칩 스테이션 내부 빠른 장착 경로에서는 호출하지 않는다 — 쓸기 중 리스트 재정렬 방지.)
	void RefreshOpenChipStationWidget();

	void RefreshLootDropWidgetForSource(ALSLootBox* SourceLootBox, const TArray<FLSDropResult>& Results);
	void SyncRaidInventoryToClient();
	void RequestRaidEntryDataForRaidStart();
	void RequestRaidResultSave(ELSRaidResult Result, const TArray<FLSSessionItem>& InventoryItems, const TArray<FLSSessionItem>& SafeItems, bool bSaveInventory, bool bSaveSafeStash);
	void ClearSubmittedRaidEntryData();
	bool HasSubmittedRaidEntryData() const { return bHasSubmittedRaidEntryData; }
	const TArray<FLSSessionItem>& GetSubmittedRaidLoadout() const { return SubmittedRaidLoadout; }
	const TArray<FLSSessionItem>& GetSubmittedRaidSafeItems() const { return SubmittedRaidSafeItems; }

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

	UFUNCTION(Client, Reliable)
	void ClientStartRaidSession(const TArray<FLSSessionItem>& Loadout, const TArray<FLSSessionItem>& SafeItems);

	bool TransferInventorySlotToLootDrop(ELSInventorySlotArea FromSlotArea, int32 FromSlotIndex);
	bool TransferInventorySlotToOpenContainer(ELSInventorySlotArea FromSlotArea, int32 FromSlotIndex, bool bRefreshOpenContainer = true);
	bool DropInventorySlot(ELSInventorySlotArea FromArea, int32 FromIndex, ELSInventorySlotArea ToArea, int32 ToIndex);
	bool DropLootDropSlot(ALSLootBox* SourceLootBox, int32 FromLootSlotIndex, int32 ToLootSlotIndex);
	bool SortRaidInventory();
	bool TransferLootDropSlotToSession(ALSLootBox* SourceLootBox, int32 LootSlotIndex, FName ItemRowName, int32 Amount, FLSSessionItem& OutLootItem);
	bool TransferLootDropSlotToSessionSlot(ALSLootBox* SourceLootBox, int32 LootSlotIndex, FName ItemRowName, int32 Amount, ELSInventorySlotArea ToSlotArea, int32 ToSlotIndex, FLSSessionItem& OutLootItem);
	bool TransferSessionSlotToLootDropSlot(ALSLootBox* SourceLootBox, ELSInventorySlotArea FromSlotArea, int32 FromSlotIndex, int32 LootSlotIndex, const FLSDropResult& CurrentLootItem, FLSSessionItem& OutLootItem);
	bool DropSessionSlotToWorld(ELSInventorySlotArea SlotArea, int32 SlotIndex, TSubclassOf<ALSWorldDroppedItem> DroppedItemClass, FVector DropDirection);
	bool DropOverflowInventorySlotsToWorld(TSubclassOf<ALSWorldDroppedItem> DroppedItemClass, FVector DropDirection);
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

	UPROPERTY(EditDefaultsOnly, Category="LS/UI")
	TSubclassOf<ULSBackgroundBlurWidget> BackgroundBlurWidgetClass;

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
	TObjectPtr<ULSBackgroundBlurWidget> BackgroundBlurWidgetInstance;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSLobbyStorageWidget> LobbyStorageWidgetInstance;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSChipStationWidget> ChipStationWidgetInstance;

	// 로비 오버레이에 배치돼 PC에 등록된 인벤토리 위젯(폰 없는 로비용). 폰이 있으면 사용하지 않는다.
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSInventoryWidget> LobbyInventoryWidgetInstance;

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

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="LS/Inventory")
	bool bHasSubmittedRaidEntryData = false;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="LS/Inventory")
	TArray<FLSSessionItem> SubmittedRaidLoadout;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="LS/Inventory")
	TArray<FLSSessionItem> SubmittedRaidSafeItems;

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
	void ServerDropOverflowInventorySlotsToWorld(TSubclassOf<ALSWorldDroppedItem> DroppedItemClass, FVector_NetQuantizeNormal DropDirection);

	UFUNCTION(Server, Reliable)
	void ServerTransferLootDropSlotToSession(ALSLootBox* SourceLootBox, int32 LootSlotIndex);

	UFUNCTION(Server, Reliable)
	void ServerTransferLootDropSlotToSessionSlot(ALSLootBox* SourceLootBox, int32 LootSlotIndex, ELSInventorySlotArea ToSlotArea, int32 ToSlotIndex);

	UFUNCTION(Server, Reliable)
	void ServerTransferSessionSlotToLootDropSlot(ALSLootBox* SourceLootBox, ELSInventorySlotArea FromSlotArea, int32 FromSlotIndex, int32 LootSlotIndex);

	UFUNCTION(Server, Reliable)
	void ServerDropInventorySlot(ELSInventorySlotArea FromArea, int32 FromIndex, ELSInventorySlotArea ToArea, int32 ToIndex);

	UFUNCTION(Server, Reliable)
	void ServerDropLootDropSlot(ALSLootBox* SourceLootBox, int32 FromLootSlotIndex, int32 ToLootSlotIndex);

	UFUNCTION(Server, Reliable)
	void ServerSortRaidInventory();

	UFUNCTION(Client, Reliable)
	void ClientRequestRaidEntryData();

	UFUNCTION(Server, Reliable)
	void ServerSubmitRaidEntryData(const TArray<FLSSessionItem>& Loadout, const TArray<FLSSessionItem>& SafeItems);

	UFUNCTION(Client, Reliable)
	void ClientApplyRaidResult(ELSRaidResult Result, const TArray<FLSSessionItem>& InventoryItems, const TArray<FLSSessionItem>& SafeItems, bool bSaveInventory, bool bSaveSafeStash);

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
	void ClientSyncRaidSessionAndLoot(ALSLootBox* SourceLootBox, const TArray<FLSSessionItem>& InventoryItems, const TArray<FLSSessionItem>& SafeItems, const TArray<FLSDropResult>& LootResults);

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
	void CreateBackgroundBlurWidgetLocal();
	void InitializeRaidInventoryFromSessionSubsystem();
	void SubmitLocalRaidEntryData();
	void StoreSubmittedRaidEntryData(const TArray<FLSSessionItem>& Loadout, const TArray<FLSSessionItem>& SafeItems);
	void ApplyRaidResultToLocalSave(ELSRaidResult Result, const TArray<FLSSessionItem>& InventoryItems, const TArray<FLSSessionItem>& SafeItems, bool bSaveInventory, bool bSaveSafeStash);
	void SyncRaidSessionAndLootFromServer(ALSLootBox* SourceLootBox);
	// 로비 파밍용: 레이드 세션이 비활성(=로비)이면 룻박스 아이템을 세이브에 직접 적재한다.
	bool IsLobbyLootMode() const;
	ULSSaveSubsystem* ResolveSaveSubsystem() const;
	void HandleNoiseForHUD(FVector NoiseLocation, float RadiusCm, FGameplayTag NoiseTag, AActor* NoiseInstigator);
	void ShowDamageNumberLocal(const FLSDamageNumberPayload& Payload);
	void SetProtocolTestLevel(ELSProtocolType ProtocolType, int32 Level);
	void RefreshProtocolTestTargets();
	bool DropSessionSlotToWorldInternal(ELSInventorySlotArea SlotArea, int32 SlotIndex, TSubclassOf<ALSWorldDroppedItem> DroppedItemClass, FVector DropDirection);
	bool DropOverflowInventorySlotsToWorldInternal(TSubclassOf<ALSWorldDroppedItem> DroppedItemClass, FVector DropDirection);
	bool SpawnDroppedItemToWorld(const FLSSessionItem& SlotItem, TSubclassOf<ALSWorldDroppedItem> DroppedItemClass, FVector DropDirection);
	bool ResolveServerDroppedItemTransform(FTransform& OutDropTransform, FVector DropDirection) const;
	bool TransferLootDropSlotToSessionInternal(ALSLootBox* SourceLootBox, int32 LootSlotIndex, FLSSessionItem& OutLootItem);
	bool TransferLootDropSlotToSessionSlotInternal(ALSLootBox* SourceLootBox, int32 LootSlotIndex, ELSInventorySlotArea ToSlotArea, int32 ToSlotIndex, FLSSessionItem& OutLootItem);
	bool TransferSessionSlotToLootDropSlotInternal(ALSLootBox* SourceLootBox, ELSInventorySlotArea FromSlotArea, int32 FromSlotIndex, int32 LootSlotIndex, FLSSessionItem& OutLootItem);
	bool DropLootDropSlotInternal(ALSLootBox* SourceLootBox, int32 FromLootSlotIndex, int32 ToLootSlotIndex);
};
