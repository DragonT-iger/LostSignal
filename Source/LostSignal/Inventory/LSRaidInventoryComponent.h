#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Session/LSSessionSubsystem.h"
#include "LSRaidInventoryComponent.generated.h"

UCLASS(ClassGroup=(LS), meta=(BlueprintSpawnableComponent))
class LOSTSIGNAL_API ULSRaidInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSRaidInventoryComponent();

	void StartRaidInventory(const TArray<FLSSessionItem>& Loadout, const TArray<FLSSessionItem>& SafeItems);
	void MirrorRaidInventoryState(const TArray<FLSSessionItem>& InventoryItems, const TArray<FLSSessionItem>& SafeItems);
	void EndRaidInventory();

	bool IsRaidActive() const { return bRaidActive; }
	int32 GetMaxInventorySlotCount() const;

	const TArray<FLSSessionItem>& GetSessionInventory() const { return SessionInventory; }
	const TArray<FLSSessionItem>& GetSessionSafeInventory() const { return SessionSafeInventory; }

	bool TryAddSessionItem(FName ItemRowName, int32 Amount, FLSSessionItem& OutRemainingItem);
	void SortSessionInventory();
	bool DropSessionSlot(ELSInventorySlotArea FromArea, int32 FromIndex, ELSInventorySlotArea ToArea, int32 ToIndex);
	bool DropExternalItemToSessionSlot(FLSSessionItem& InOutExternalItem, ELSInventorySlotArea ToArea, int32 ToIndex);
	bool GetSessionSlotItem(ELSInventorySlotArea SlotArea, int32 SlotIndex, FLSSessionItem& OutItem) const;
	bool ClearSessionSlot(ELSInventorySlotArea SlotArea, int32 SlotIndex);
	bool ReplaceSessionSlotItem(ELSInventorySlotArea SlotArea, int32 SlotIndex, const FLSSessionItem& NewItem, FLSSessionItem& OutPreviousItem);

private:
	bool bRaidActive = false;
	TArray<FLSSessionItem> SessionInventory;
	TArray<FLSSessionItem> SessionSafeInventory;
	TArray<FLSSessionItem> ConsumedItems;
	FLSLoadoutSnapshot LoadoutSnapshot;
};
