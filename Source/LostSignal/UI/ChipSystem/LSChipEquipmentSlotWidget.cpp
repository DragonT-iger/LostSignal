#include "UI/ChipSystem/LSChipEquipmentSlotWidget.h"

#include "LostSignal.h"
#include "UI/ChipSystem/LSChipStationWidget.h"
#include "UI/Inventory/LSInventoryDragDropOperation.h"
#include "UI/Inventory/LSItemSlotWidget.h"

void ULSChipEquipmentSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!ItemSlot)
	{
		UE_LOG(LogLS, Warning, TEXT("ItemSlot is not bound on %s."), *GetNameSafe(this));
		return;
	}

	ItemSlot->SetChipEquipmentSlotContext(this, ChipStationWidget.Get(), EquipmentSlotIndex);
	ItemSlot->ClearItem();
}

void ULSChipEquipmentSlotWidget::SetEquipmentSlotContext(ULSChipStationWidget* InChipStationWidget, const int32 InEquipmentSlotIndex)
{
	ChipStationWidget = InChipStationWidget;
	EquipmentSlotIndex = InEquipmentSlotIndex;

	if (!ItemSlot)
	{
		UE_LOG(LogLS, Warning, TEXT("ItemSlot is not bound on %s."), *GetNameSafe(this));
		return;
	}

	ItemSlot->SetChipEquipmentSlotContext(this, ChipStationWidget.Get(), EquipmentSlotIndex);
	ItemSlot->ClearItem();
}

bool ULSChipEquipmentSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	ULSInventoryDragDropOperation* DragOperation = Cast<ULSInventoryDragDropOperation>(InOperation);
	if (!DragOperation)
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	return HandleChipDrop(*DragOperation);
}

bool ULSChipEquipmentSlotWidget::HandleChipDrop(const ULSInventoryDragDropOperation& DragOperation)
{
	if (!DragOperation.SourceChipStationWidget || DragOperation.DragItemRowName.IsNone() || DragOperation.DragAmount <= 0)
	{
		return false;
	}

	if (ChipStationWidget.IsValid() && DragOperation.SourceChipStationWidget != ChipStationWidget.Get())
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot equip chip because source chip station does not match on %s."), *GetNameSafe(this));
		return false;
	}

	if (!ChipStationWidget.IsValid())
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot equip chip because chip station context is missing on %s."), *GetNameSafe(this));
		return false;
	}

	if (!DragOperation.DragItemRowName.ToString().StartsWith(TEXT("Chip_")))
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot equip non-chip item '%s' on %s."), *DragOperation.DragItemRowName.ToString(), *GetNameSafe(this));
		return false;
	}

	if (DragOperation.SourceChipEquipmentSlotWidget)
	{
		const bool bDropped = ChipStationWidget->DropEquippedChipToHardwareSlot(DragOperation, EquipmentSlotIndex);
		if (bDropped)
		{
			UE_LOG(LogLS, Log, TEXT("Dropped equipped chip '%s' from hardware slot %d to hardware slot %d on %s."),
				*DragOperation.DragItemRowName.ToString(),
				DragOperation.SourceEquipmentSlotIndex,
				EquipmentSlotIndex,
				*GetNameSafe(this));
		}

		return bDropped;
	}

	const bool bEquipped = ChipStationWidget->EquipChipToHardwareSlot(DragOperation, EquipmentSlotIndex);
	if (bEquipped)
	{
		UE_LOG(LogLS, Log, TEXT("Equipped chip '%s' to hardware slot %d on %s."),
			*DragOperation.DragItemRowName.ToString(),
			EquipmentSlotIndex,
			*GetNameSafe(this));
	}

	return bEquipped;
}

void ULSChipEquipmentSlotWidget::SetEquipmentItem(const FLSSessionItem& Item)
{
	if (!ItemSlot)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot set equipment item because ItemSlot is not bound on %s."), *GetNameSafe(this));
		return;
	}

	ItemSlot->SetItem(Item.ItemRowName, Item.Amount, Item.ChipStats);
	ItemSlot->SetChipEquipmentSlotContext(this, ChipStationWidget.Get(), EquipmentSlotIndex);
}

void ULSChipEquipmentSlotWidget::ClearEquipmentSlot()
{
	if (!ItemSlot)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot clear equipment slot because ItemSlot is not bound on %s."), *GetNameSafe(this));
		return;
	}

	ItemSlot->ClearItem();
	ItemSlot->SetChipEquipmentSlotContext(this, ChipStationWidget.Get(), EquipmentSlotIndex);
}
