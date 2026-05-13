// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/LSDropSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Session/LSSessionSubsystem.h"
#include "LSPlayerControllerBase.generated.h"

class ALSWorldDroppedItem;
class ALSLootBox;
class UInputMappingContext;
class ULSHpDebugWidget;
class ULSLootDropWidget;

UCLASS(Abstract)
class LOSTSIGNAL_API ALSPlayerControllerBase : public APlayerController
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="LS/UI")
	TSubclassOf<ULSHpDebugWidget> GetDebugHpWidgetClass() const { return DebugHpWidgetClass; }

	UFUNCTION(BlueprintCallable, Category="LS/UI")
	void ShowLootDropWidget(const FText& LootSourceName, const TArray<FLSDropResult>& Results, ALSLootBox* SourceLootBox);

	UFUNCTION(BlueprintCallable, Category="LS/UI")
	void HideLootDropWidget();

	UFUNCTION(BlueprintCallable, Category="LS/UI")
	bool TransferHoveredLootDropItemToInventory();

	bool TransferInventorySlotToLootDrop(ELSInventorySlotArea FromSlotArea, int32 FromSlotIndex);
	bool TransferLootDropSlotToSession(ALSLootBox* SourceLootBox, int32 LootSlotIndex, FName ItemRowName, int32 Amount, FLSSessionItem& OutLootItem);
	bool TransferLootDropSlotToSessionSlot(ALSLootBox* SourceLootBox, int32 LootSlotIndex, FName ItemRowName, int32 Amount, ELSInventorySlotArea ToSlotArea, int32 ToSlotIndex, FLSSessionItem& OutLootItem);
	bool TransferSessionSlotToLootDropSlot(ALSLootBox* SourceLootBox, ELSInventorySlotArea FromSlotArea, int32 FromSlotIndex, int32 LootSlotIndex, const FLSDropResult& CurrentLootItem, FLSSessionItem& OutLootItem);
	bool DropSessionSlotToWorld(ELSInventorySlotArea SlotArea, int32 SlotIndex, TSubclassOf<ALSWorldDroppedItem> DroppedItemClass, const FVector& DropLocation, float DropYaw);

protected:
	UPROPERTY(EditAnywhere, Category="LS/Input")
	TArray<TObjectPtr<UInputMappingContext>> DefaultMappingContexts;

	UPROPERTY(EditDefaultsOnly, Category="LS/UI")
	TSubclassOf<ULSHpDebugWidget> DebugHpWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="LS/UI")
	TSubclassOf<ULSLootDropWidget> LootDropWidgetClass;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSHpDebugWidget> DebugHpWidgetInstance;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="LS/UI")
	TObjectPtr<ULSLootDropWidget> LootDropWidgetInstance;

	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="LS/Input")
	bool bDefaultMappingContextsApplied = false;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

private:
	UFUNCTION(Server, Reliable)
	void ServerDropSessionSlotToWorld(ELSInventorySlotArea SlotArea, int32 SlotIndex, TSubclassOf<ALSWorldDroppedItem> DroppedItemClass, FVector_NetQuantize DropLocation, float DropYaw);

	UFUNCTION(Server, Reliable)
	void ServerTransferLootDropSlotToSession(ALSLootBox* SourceLootBox, int32 LootSlotIndex);

	UFUNCTION(Server, Reliable)
	void ServerTransferLootDropSlotToSessionSlot(ALSLootBox* SourceLootBox, int32 LootSlotIndex, ELSInventorySlotArea ToSlotArea, int32 ToSlotIndex);

	UFUNCTION(Server, Reliable)
	void ServerTransferSessionSlotToLootDropSlot(ALSLootBox* SourceLootBox, ELSInventorySlotArea FromSlotArea, int32 FromSlotIndex, int32 LootSlotIndex);

	UFUNCTION(Client, Reliable)
	void ClientShowLootDropWidget(const FText& LootSourceName, const TArray<FLSDropResult>& Results, ALSLootBox* SourceLootBox);

	UFUNCTION(Client, Reliable)
	void ClientHideLootDropWidget();

	void ShowLootDropWidgetLocal(const FText& LootSourceName, const TArray<FLSDropResult>& Results, ALSLootBox* SourceLootBox);
	void HideLootDropWidgetLocal();
	bool DropSessionSlotToWorldInternal(ELSInventorySlotArea SlotArea, int32 SlotIndex, TSubclassOf<ALSWorldDroppedItem> DroppedItemClass, const FVector& DropLocation, float DropYaw);
	bool TransferLootDropSlotToSessionInternal(ALSLootBox* SourceLootBox, int32 LootSlotIndex, FLSSessionItem& OutLootItem);
	bool TransferLootDropSlotToSessionSlotInternal(ALSLootBox* SourceLootBox, int32 LootSlotIndex, ELSInventorySlotArea ToSlotArea, int32 ToSlotIndex, FLSSessionItem& OutLootItem);
	bool TransferSessionSlotToLootDropSlotInternal(ALSLootBox* SourceLootBox, ELSInventorySlotArea FromSlotArea, int32 FromSlotIndex, int32 LootSlotIndex, FLSSessionItem& OutLootItem);
	bool TransferExternalLootItemToSession(FName ItemRowName, int32 Amount, FLSSessionItem& OutLootItem);
	bool TransferExternalLootItemToSessionSlot(FName ItemRowName, int32 Amount, ELSInventorySlotArea ToSlotArea, int32 ToSlotIndex, FLSSessionItem& OutLootItem);
	bool TransferSessionSlotToExternalLootSlot(ELSInventorySlotArea FromSlotArea, int32 FromSlotIndex, const FLSDropResult& CurrentLootItem, FLSSessionItem& OutLootItem);
};
