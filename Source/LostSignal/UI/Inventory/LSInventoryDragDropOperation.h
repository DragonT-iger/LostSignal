#pragma once

#include "Blueprint/DragDropOperation.h"
#include "CoreMinimal.h"
#include "Session/LSSessionSubsystem.h"
#include "LSInventoryDragDropOperation.generated.h"

class ULSInventoryWidget;
class ULSItemSlotWidget;
class ULSLobbyStorageWidget;
class ULSLootDropWidget;
class ULSChipStationWidget;
class ULSChipEquipmentSlotWidget;

UCLASS()
class LOSTSIGNAL_API ULSInventoryDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<ULSInventoryWidget> SourceInventoryWidget;

	UPROPERTY()
	TObjectPtr<ULSLootDropWidget> SourceLootDropWidget;

	UPROPERTY()
	TObjectPtr<ULSLobbyStorageWidget> SourceLobbyStorageWidget;

	UPROPERTY()
	TObjectPtr<ULSChipStationWidget> SourceChipStationWidget;

	UPROPERTY()
	TObjectPtr<ULSChipEquipmentSlotWidget> SourceChipEquipmentSlotWidget;

	UPROPERTY()
	TObjectPtr<ULSItemSlotWidget> SourceSlotWidget;

	UPROPERTY()
	int32 SourceSlotIndex = INDEX_NONE;

	UPROPERTY()
	int32 SourceEquipmentSlotIndex = INDEX_NONE;

	UPROPERTY()
	ELSInventorySlotArea SourceSlotArea = ELSInventorySlotArea::Inventory;

	UPROPERTY()
	FName DragItemRowName;

	UPROPERTY()
	int32 DragAmount = 0;

	UPROPERTY()
	TArray<FLSChipResolvedStat> DragChipStats;

	virtual void DragCancelled_Implementation(const FPointerEvent& PointerEvent) override;
	virtual void Drop_Implementation(const FPointerEvent& PointerEvent) override;
};
