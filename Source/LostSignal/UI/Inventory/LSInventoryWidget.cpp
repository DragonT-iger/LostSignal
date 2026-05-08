#include "UI/Inventory/LSInventoryWidget.h"

#include "LostSignal.h"
#include "Components/Button.h"
#include "Components/WrapBox.h"
#include "Session/LSSaveSubsystem.h"
#include "Session/LSSessionSubsystem.h"
#include "UI/Inventory/LSInventoryItemSlotWidget.h"

namespace
{
void AppendSlotItems(TArray<FLSSessionItem>& Items, const TArray<FLSSessionItem>& NewItems)
{
	for (const FLSSessionItem& NewItem : NewItems)
	{
		if (NewItem.ItemRowName.IsNone() || NewItem.Amount <= 0)
		{
			UE_LOG(LogLS, Warning, TEXT("Skipping invalid inventory slot item. Row=%s Amount=%d"), *NewItem.ItemRowName.ToString(), NewItem.Amount);
			continue;
		}

		Items.Add(NewItem);
	}
}
}

void ULSInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!StoreAllButton)
	{
		UE_LOG(LogLS, Warning, TEXT("StoreAllButton is not bound on %s."), *GetNameSafe(this));
	}
	else
	{
		StoreAllButton->OnClicked.AddDynamic(this, &ULSInventoryWidget::HandleStoreAllButtonClicked);
	}

	if (!SortButton)
	{
		UE_LOG(LogLS, Warning, TEXT("SortButton is not bound on %s."), *GetNameSafe(this));
	}
	else
	{
		SortButton->OnClicked.AddDynamic(this, &ULSInventoryWidget::HandleSortButtonClicked);
	}

	RebuildInventorySlots();
	RebuildConfirmedStorageSlots();
}

void ULSInventoryWidget::NativeDestruct()
{
	if (StoreAllButton)
	{
		StoreAllButton->OnClicked.RemoveDynamic(this, &ULSInventoryWidget::HandleStoreAllButtonClicked);
	}

	if (SortButton)
	{
		SortButton->OnClicked.RemoveDynamic(this, &ULSInventoryWidget::HandleSortButtonClicked);
	}

	Super::NativeDestruct();
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
		if (ULSSessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<ULSSessionSubsystem>())
		{
			if (SessionSubsystem->IsRaidActive())
			{
				AppendSlotItems(InventoryItems, SessionSubsystem->GetSessionInventory());
			}
			else if (ULSSaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<ULSSaveSubsystem>())
			{
				AppendSlotItems(InventoryItems, SaveSubsystem->GetStash());
			}
			else
			{
				UE_LOG(LogLS, Warning, TEXT("SaveSubsystem is missing on %s."), *GetNameSafe(this));
			}
		}
		else
		{
			UE_LOG(LogLS, Warning, TEXT("SessionSubsystem is missing on %s."), *GetNameSafe(this));
			if (ULSSaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<ULSSaveSubsystem>())
			{
				AppendSlotItems(InventoryItems, SaveSubsystem->GetStash());
			}
		}

		UE_LOG(LogLS, Log, TEXT("InventoryWidget rebuilt with %d slot items on %s."), InventoryItems.Num(), *GetNameSafe(this));
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
			SlotWidget->SetSlotContext(this, SlotIndex, InventoryItems.IsValidIndex(SlotIndex));

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

bool ULSInventoryWidget::HandleInventorySlotDrop(const int32 FromSlotIndex, const int32 ToSlotIndex, const bool bMoveOperation)
{
	if (FromSlotIndex == INDEX_NONE || ToSlotIndex == INDEX_NONE)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot handle inventory slot drop because an index is invalid. From=%d To=%d"), FromSlotIndex, ToSlotIndex);
		return false;
	}

	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot handle inventory slot drop because GameInstance is missing on %s."), *GetNameSafe(this));
		return false;
	}

	ULSSessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<ULSSessionSubsystem>();
	if (!SessionSubsystem)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot handle inventory slot drop because SessionSubsystem is missing on %s."), *GetNameSafe(this));
		return false;
	}

	if (!SessionSubsystem->IsRaidActive())
	{
		UE_LOG(LogLS, Warning, TEXT("Inventory slot drag/drop is only supported during an active raid on %s."), *GetNameSafe(this));
		return false;
	}

	const bool bChanged = bMoveOperation
		? SessionSubsystem->MoveSessionInventorySlot(FromSlotIndex, ToSlotIndex)
		: SessionSubsystem->SwapSessionInventorySlots(FromSlotIndex, ToSlotIndex);

	UE_LOG(LogLS, Log, TEXT("Inventory slot drop handled on %s. Mode=%s From=%d To=%d Changed=%s"),
		*GetNameSafe(this),
		bMoveOperation ? TEXT("Move") : TEXT("Swap"),
		FromSlotIndex,
		ToSlotIndex,
		bChanged ? TEXT("true") : TEXT("false"));

	if (bChanged)
	{
		RebuildInventorySlots();
	}

	return bChanged;
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
			SlotWidget->SetSlotContext(this, INDEX_NONE, false);
			SlotWidget->ClearItem();
			ConfirmedStorageSlotWrapBox->AddChildToWrapBox(SlotWidget);
		}
		else
		{
			UE_LOG(LogLS, Warning, TEXT("Failed to create confirmed storage slot widget at index %d on %s."), SlotIndex, *GetNameSafe(this));
		}
	}
}

void ULSInventoryWidget::HandleStoreAllButtonClicked()
{
	UE_LOG(LogLS, Warning, TEXT("StoreAllButton clicked on %s, but store-all behavior is not implemented yet."), *GetNameSafe(this));
}

void ULSInventoryWidget::HandleSortButtonClicked()
{
	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot sort inventory because GameInstance is missing on %s."), *GetNameSafe(this));
		return;
	}

	if (ULSSessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<ULSSessionSubsystem>())
	{
		if (SessionSubsystem->IsRaidActive())
		{
			SessionSubsystem->SortSessionInventory();
		}
		else if (ULSSaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<ULSSaveSubsystem>())
		{
			SaveSubsystem->SortStash();
		}
		else
		{
			UE_LOG(LogLS, Warning, TEXT("Cannot sort stash because SaveSubsystem is missing on %s."), *GetNameSafe(this));
		}
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot sort session inventory because SessionSubsystem is missing on %s."), *GetNameSafe(this));
	}

	RebuildInventorySlots();
	RebuildConfirmedStorageSlots();
}
