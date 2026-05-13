#include "UI/Inventory/LSInventoryDragDropOperation.h"

#include "UI/Inventory/LSInventoryWidget.h"
#include "UI/Inventory/LSItemSlotWidget.h"

void ULSInventoryDragDropOperation::DragCancelled_Implementation(const FPointerEvent& PointerEvent)
{
	if (SourceInventoryWidget)
	{
		SourceInventoryWidget->TryDropInventoryDragToWorld(*this, PointerEvent);
	}

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
