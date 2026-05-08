#include "UI/Inventory/LSInventoryDragDropOperation.h"

#include "UI/Inventory/LSInventoryItemSlotWidget.h"

void ULSInventoryDragDropOperation::DragCancelled_Implementation(const FPointerEvent& PointerEvent)
{
	if (SourceSlotWidget)
	{
		SourceSlotWidget->RestoreDragSourceVisual();
	}
}

void ULSInventoryDragDropOperation::Drop_Implementation(const FPointerEvent& PointerEvent)
{
	if (SourceSlotWidget)
	{
		SourceSlotWidget->RestoreDragSourceVisual();
	}
}
