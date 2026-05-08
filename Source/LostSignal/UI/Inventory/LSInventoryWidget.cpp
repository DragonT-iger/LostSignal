#include "UI/Inventory/LSInventoryWidget.h"

#include "LostSignal.h"
#include "Components/WrapBox.h"
#include "Session/LSSaveSubsystem.h"
#include "Session/LSSessionSubsystem.h"
#include "UI/Inventory/LSInventoryItemSlotWidget.h"

namespace
{
void MergeInventoryItem(TArray<FLSSessionItem>& Items, const FLSSessionItem& NewItem)
{
	if (NewItem.ItemRowName.IsNone() || NewItem.Amount <= 0)
	{
		return;
	}

	for (FLSSessionItem& ExistingItem : Items)
	{
		if (ExistingItem.ItemRowName == NewItem.ItemRowName)
		{
			ExistingItem.Amount += NewItem.Amount;
			return;
		}
	}

	Items.Add(NewItem);
}

void MergeInventoryItems(TArray<FLSSessionItem>& Items, const TArray<FLSSessionItem>& NewItems)
{
	for (const FLSSessionItem& NewItem : NewItems)
	{
		MergeInventoryItem(Items, NewItem);
	}
}
}

void ULSInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RebuildInventorySlots();
	RebuildConfirmedStorageSlots();
}

void ULSInventoryWidget::SetInventorySlotCount(const int32 NewInventorySlotCount)
{
	InventorySlotCount = FMath::Max(0, NewInventorySlotCount);
	RebuildInventorySlots();
}

void ULSInventoryWidget::SetConfirmedStorageSlotCount(const int32 NewConfirmedStorageSlotCount)
{
	ConfirmedStorageSlotCount = FMath::Max(0, NewConfirmedStorageSlotCount);
	RebuildConfirmedStorageSlots();
}

void ULSInventoryWidget::RebuildInventorySlots()
{
	if (!InventoryWrapBox)
	{
		UE_LOG(LogLS, Warning, TEXT("InventoryWrapBox is not bound on %s."), *GetNameSafe(this));
		return;
	}

	InventoryWrapBox->ClearChildren();

	if (!InventoryItemSlotWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("InventoryItemSlotWidgetClass is not set on %s."), *GetNameSafe(this));
		return;
	}

	APlayerController* OwningPlayer = GetOwningPlayer();
	UWorld* World = GetWorld();
	if (!OwningPlayer && !World)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot rebuild inventory slots because owner/world is missing on %s."), *GetNameSafe(this));
		return;
	}

	TArray<FLSSessionItem> InventoryItems;
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (ULSSaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<ULSSaveSubsystem>())
		{
			MergeInventoryItems(InventoryItems, SaveSubsystem->GetStash());
		}
		else
		{
			UE_LOG(LogLS, Warning, TEXT("SaveSubsystem is missing on %s."), *GetNameSafe(this));
		}

		if (ULSSessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<ULSSessionSubsystem>())
		{
			MergeInventoryItems(InventoryItems, SessionSubsystem->GetSessionInventory());
		}
		else
		{
			UE_LOG(LogLS, Warning, TEXT("SessionSubsystem is missing on %s."), *GetNameSafe(this));
		}

		UE_LOG(LogLS, Log, TEXT("InventoryWidget rebuilt with %d merged items on %s."), InventoryItems.Num(), *GetNameSafe(this));
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("GameInstance is missing on %s."), *GetNameSafe(this));
	}

	const int32 SlotCountToBuild = FMath::Max(InventorySlotCount, InventoryItems.Num());
	for (int32 SlotIndex = 0; SlotIndex < SlotCountToBuild; ++SlotIndex)
	{
		ULSInventoryItemSlotWidget* SlotWidget = OwningPlayer
			? CreateWidget<ULSInventoryItemSlotWidget>(OwningPlayer, InventoryItemSlotWidgetClass)
			: CreateWidget<ULSInventoryItemSlotWidget>(World, InventoryItemSlotWidgetClass);

		if (SlotWidget)
		{
			if (InventoryItems.IsValidIndex(SlotIndex))
			{
				SlotWidget->SetItem(InventoryItems[SlotIndex].ItemRowName, InventoryItems[SlotIndex].Amount);
			}
			else
			{
				SlotWidget->ClearItem();
			}

			InventoryWrapBox->AddChildToWrapBox(SlotWidget);
		}
		else
		{
			UE_LOG(LogLS, Warning, TEXT("Failed to create inventory slot widget at index %d on %s."), SlotIndex, *GetNameSafe(this));
		}
	}
}

void ULSInventoryWidget::RebuildConfirmedStorageSlots()
{
	if (!ConfirmedStorageSlotWrapBox)
	{
		UE_LOG(LogLS, Warning, TEXT("ConfirmedStorageSlotWrapBox is not bound on %s."), *GetNameSafe(this));
		return;
	}

	ConfirmedStorageSlotWrapBox->ClearChildren();

	if (!InventoryItemSlotWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("InventoryItemSlotWidgetClass is not set on %s. Confirmed storage slots use the same widget class."), *GetNameSafe(this));
		return;
	}

	APlayerController* OwningPlayer = GetOwningPlayer();
	UWorld* World = GetWorld();
	if (!OwningPlayer && !World)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot rebuild confirmed storage slots because owner/world is missing on %s."), *GetNameSafe(this));
		return;
	}

	for (int32 SlotIndex = 0; SlotIndex < ConfirmedStorageSlotCount; ++SlotIndex)
	{
		ULSInventoryItemSlotWidget* SlotWidget = OwningPlayer
			? CreateWidget<ULSInventoryItemSlotWidget>(OwningPlayer, InventoryItemSlotWidgetClass)
			: CreateWidget<ULSInventoryItemSlotWidget>(World, InventoryItemSlotWidgetClass);

		if (SlotWidget)
		{
			SlotWidget->ClearItem();
			ConfirmedStorageSlotWrapBox->AddChildToWrapBox(SlotWidget);
		}
		else
		{
			UE_LOG(LogLS, Warning, TEXT("Failed to create confirmed storage slot widget at index %d on %s."), SlotIndex, *GetNameSafe(this));
		}
	}
}
