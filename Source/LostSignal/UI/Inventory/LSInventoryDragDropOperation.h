#pragma once

#include "Blueprint/DragDropOperation.h"
#include "CoreMinimal.h"
#include "Session/LSSessionSubsystem.h"
#include "LSInventoryDragDropOperation.generated.h"

class ULSInventoryWidget;
class ULSItemSlotWidget;

UCLASS()
class LOSTSIGNAL_API ULSInventoryDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<ULSInventoryWidget> SourceInventoryWidget;

	UPROPERTY()
	TObjectPtr<ULSItemSlotWidget> SourceSlotWidget;

	UPROPERTY()
	int32 SourceSlotIndex = INDEX_NONE;

	UPROPERTY()
	ELSInventorySlotArea SourceSlotArea = ELSInventorySlotArea::Inventory;

	virtual void DragCancelled_Implementation(const FPointerEvent& PointerEvent) override;
	virtual void Drop_Implementation(const FPointerEvent& PointerEvent) override;
};
