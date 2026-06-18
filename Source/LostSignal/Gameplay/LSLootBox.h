#pragma once

#include "CoreMinimal.h"
#include "Gameplay/LSInteractableObject.h"
#include "Data/LSDropSubsystem.h"
#include "Session/LSSessionSubsystem.h"
#include "LSLootBox.generated.h"

class ULSRaidInventoryComponent;
class ULSSaveSubsystem;
class ULSMinimapMarkerComponent;

UCLASS()
class LOSTSIGNAL_API ALSLootBox : public ALSInteractableObject
{
	GENERATED_BODY()

public:
	ALSLootBox();

	virtual bool CanInteract_Implementation(APawn* Interactor) override;
	virtual void Interact_Implementation(APawn* Interactor) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	const TArray<FLSDropResult>& GetLootResults() const { return LootResults; }
	bool DropLootSlot(int32 FromLootSlotIndex, int32 ToLootSlotIndex);
	bool TransferLootSlotToSession(int32 LootSlotIndex, ULSRaidInventoryComponent* RaidInventory, FLSSessionItem& OutRemainingLootItem);
	bool TransferLootSlotToSessionSlot(int32 LootSlotIndex, ULSRaidInventoryComponent* RaidInventory, ELSInventorySlotArea ToSlotArea, int32 ToSlotIndex, FLSSessionItem& OutRemainingLootItem);
	bool TransferSessionSlotToLootSlot(int32 LootSlotIndex, ULSRaidInventoryComponent* RaidInventory, ELSInventorySlotArea FromSlotArea, int32 FromSlotIndex, FLSSessionItem& OutLootItem);

	// 로비 파밍용: 레이드 세션이 없을 때 룻박스 아이템을 영구 세이브(SaveSubsystem)에 직접 전송한다.
	bool TransferLootSlotToSave(int32 LootSlotIndex, ULSSaveSubsystem* SaveSubsystem, FLSSessionItem& OutRemainingLootItem);
	bool TransferLootSlotToSaveSlot(int32 LootSlotIndex, ULSSaveSubsystem* SaveSubsystem, ELSInventorySlotArea ToSlotArea, int32 ToSlotIndex, FLSSessionItem& OutRemainingLootItem);
	bool TransferSaveSlotToLootSlot(int32 LootSlotIndex, ULSSaveSubsystem* SaveSubsystem, ELSInventorySlotArea FromSlotArea, int32 FromSlotIndex, FLSSessionItem& OutLootItem);

	UFUNCTION(BlueprintImplementableEvent, Category="LS/Loot")
	void OnLootResultReceived(const TArray<FLSDropResult>& Results);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LS/Loot")
	FName RootingObjectRowName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Minimap")
	TObjectPtr<ULSMinimapMarkerComponent> MinimapMarkerComponent;

private:
	UPROPERTY(ReplicatedUsing=OnRep_IsOpened, VisibleInstanceOnly, Category="LS/Loot")
	bool bIsOpened = false;

	UPROPERTY(ReplicatedUsing=OnRep_LootResults, Transient, VisibleInstanceOnly, Category="LS/Loot")
	TArray<FLSDropResult> LootResults;

	UFUNCTION()
	void OnRep_IsOpened();

	UFUNCTION()
	void OnRep_LootResults();

	void ClearLootSlot(int32 LootSlotIndex);
	void NotifyLootResultsChanged();
};
