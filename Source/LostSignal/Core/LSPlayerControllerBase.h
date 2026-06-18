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
class ULSChipStationWidget;
class ULSRaidInventoryComponent;
class ULSSaveSubsystem;
class ULSHpDebugWidget;
class ULSLootDropWidget;
class ULSPlayerHUDWidget;
class ULSProtocolDebugWidget;
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

	int32 GetOpenLobbyStorageMaxSlotCount() const;
	void RefreshOpenLobbyStorageWidget();

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

	// 프로토콜 디버그 패널이 현재 화면에 떠 있는지. 칩스테이션은 패널이 떠 있을 때만 오버라이드를 따른다.
	bool IsProtocolDebugWidgetVisible() const;

	bool HasSurvivalProtocolTestLevel() const { return HasProtocolTestLevel(ELSProtocolType::Survival); }
	int32 GetSurvivalProtocolTestLevel() const { return GetProtocolTestLevel(ELSProtocolType::Survival); }
	bool HasProtocolTestLevel(ELSProtocolType ProtocolType) const;
	int32 GetProtocolTestLevel(ELSProtocolType ProtocolType) const;
	// 현재 적용 중인 프로토콜 레벨: 오버라이드가 있으면 그 값, 없으면 신호 활성 장착 칩의 합산값.
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

	// 시연용 프로토콜 조정 패널 인스턴스.
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="LS/Debug")
	TObjectPtr<ULSProtocolDebugWidget> ProtocolDebugWidgetInstance;

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
