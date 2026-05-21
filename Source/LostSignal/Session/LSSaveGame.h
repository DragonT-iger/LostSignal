#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Session/LSSessionSubsystem.h"
#include "LSSaveGame.generated.h"

UCLASS()
class LOSTSIGNAL_API ULSSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	// Legacy migration-only. New code uses Inventory/WarehouseItems/SafeStash.
	UPROPERTY() TArray<FLSSessionItem> Stash;
	UPROPERTY() TArray<FLSSessionItem> Player1Inventory;
	UPROPERTY() bool bInventoryMigrated = false;

	// Slot-based storage. Duplicate ItemRowName entries are valid when an item exceeds Item_Max.
	UPROPERTY() TArray<FLSSessionItem> Inventory;
	UPROPERTY() TArray<FLSSessionItem> WarehouseItems;
	UPROPERTY() TArray<FLSSessionItem> SafeStash;

	UPROPERTY() bool bRaidSaveActive = false;
	UPROPERTY() TArray<FLSSessionItem> ActiveRaidLoadout;
	UPROPERTY() TArray<FLSSessionItem> ActiveRaidConsumedItems;
};
