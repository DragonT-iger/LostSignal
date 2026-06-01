// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/LSDropSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Session/LSSessionSubsystem.h"
#include "LSPlayerControllerBase.generated.h"

class ALSLootBox;
class ALSWorldDroppedItem;
class UInputMappingContext;
class ULSLobbyStorageWidget;
class ULSRaidInventoryComponent;
class ULSHpDebugWidget;
class ULSLootDropWidget;
class ULSPlayerHUDWidget;

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

	UFUNCTION(Client, Reliable)
	void ClientStartRaidSession(const TArray<FLSSessionItem>& Loadout, const TArray<FLSSessionItem>& SafeItems);

	bool TransferInventorySlotToLootDrop(ELSInventorySlotArea FromSlotArea, int32 FromSlotIndex);
	bool TransferInventorySlotToOpenContainer(ELSInventorySlotArea FromSlotArea, int32 FromSlotIndex);
	bool DropInventorySlot(ELSInventorySlotArea FromArea, int32 FromIndex, ELSInventorySlotArea ToArea, int32 ToIndex);
	bool DropLootDropSlot(ALSLootBox* SourceLootBox, int32 FromLootSlotIndex, int32 ToLootSlotIndex);
	bool SortRaidInventory();
	bool TransferLootDropSlotToSession(ALSLootBox* SourceLootBox, int32 LootSlotIndex, FName ItemRowName, int32 Amount, FLSSessionItem& OutLootItem);
	bool TransferLootDropSlotToSessionSlot(ALSLootBox* SourceLootBox, int32 LootSlotIndex, FName ItemRowName, int32 Amount, ELSInventorySlotArea ToSlotArea, int32 ToSlotIndex, FLSSessionItem& OutLootItem);
	bool TransferSessionSlotToLootDropSlot(ALSLootBox* SourceLootBox, ELSInventorySlotArea FromSlotArea, int32 FromSlotIndex, int32 LootSlotIndex, const FLSDropResult& CurrentLootItem, FLSSessionItem& OutLootItem);
	bool DropSessionSlotToWorld(ELSInventorySlotArea SlotArea, int32 SlotIndex, TSubclassOf<ALSWorldDroppedItem> DroppedItemClass, FVector DropDirection);
	bool ResolveDropDirectionFromSlatePosition(FVector2D SlatePosition, FVector& OutDropDirection) const;

protected:
	UPROPERTY(EditAnywhere, Category="LS/Input")
	TArray<TObjectPtr<UInputMappingContext>> DefaultMappingContexts;

	UPROPERTY(EditDefaultsOnly, Category="LS/UI")
	TSubclassOf<ULSHpDebugWidget> DebugHpWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="LS/UI")
	TSubclassOf<ULSLootDropWidget> LootDropWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="LS/UI")
	TSubclassOf<ULSPlayerHUDWidget> PlayerHUDWidgetClass;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSHpDebugWidget> DebugHpWidgetInstance;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSLootDropWidget> LootDropWidgetInstance;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSPlayerHUDWidget> PlayerHUDWidgetInstance;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSLobbyStorageWidget> LobbyStorageWidgetInstance;

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

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void AcknowledgePossession(APawn* InPawn) override;

private:
	UFUNCTION(Server, Reliable)
	void ServerDropSessionSlotToWorld(ELSInventorySlotArea SlotArea, int32 SlotIndex, TSubclassOf<ALSWorldDroppedItem> DroppedItemClass, FVector_NetQuantizeNormal DropDirection);

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
	void ClientSyncRaidSessionAndLoot(ALSLootBox* SourceLootBox, const TArray<FLSSessionItem>& InventoryItems, const TArray<FLSSessionItem>& SafeItems, const TArray<FLSDropResult>& LootResults);

	void ShowLootDropWidgetLocal(const FText& LootSourceName, const TArray<FLSDropResult>& Results, ALSLootBox* SourceLootBox);
	void HideLootDropWidgetLocal();
	void ShowLobbyStorageWidgetLocal(TSubclassOf<ULSLobbyStorageWidget> LobbyStorageWidgetClass);
	void HideLobbyStorageWidgetLocal();
	void CreatePlayerHUDWidgetLocal();
	void InitializeRaidInventoryFromSessionSubsystem();
	void SubmitLocalRaidEntryData();
	void StoreSubmittedRaidEntryData(const TArray<FLSSessionItem>& Loadout, const TArray<FLSSessionItem>& SafeItems);
	void ApplyRaidResultToLocalSave(ELSRaidResult Result, const TArray<FLSSessionItem>& InventoryItems, const TArray<FLSSessionItem>& SafeItems, bool bSaveInventory, bool bSaveSafeStash);
	void SyncRaidSessionAndLootFromServer(ALSLootBox* SourceLootBox);
	bool DropSessionSlotToWorldInternal(ELSInventorySlotArea SlotArea, int32 SlotIndex, TSubclassOf<ALSWorldDroppedItem> DroppedItemClass, FVector DropDirection);
	bool ResolveServerDroppedItemTransform(FTransform& OutDropTransform, FVector DropDirection) const;
	bool TransferLootDropSlotToSessionInternal(ALSLootBox* SourceLootBox, int32 LootSlotIndex, FLSSessionItem& OutLootItem);
	bool TransferLootDropSlotToSessionSlotInternal(ALSLootBox* SourceLootBox, int32 LootSlotIndex, ELSInventorySlotArea ToSlotArea, int32 ToSlotIndex, FLSSessionItem& OutLootItem);
	bool TransferSessionSlotToLootDropSlotInternal(ALSLootBox* SourceLootBox, ELSInventorySlotArea FromSlotArea, int32 FromSlotIndex, int32 LootSlotIndex, FLSSessionItem& OutLootItem);
	bool DropLootDropSlotInternal(ALSLootBox* SourceLootBox, int32 FromLootSlotIndex, int32 ToLootSlotIndex);
};
