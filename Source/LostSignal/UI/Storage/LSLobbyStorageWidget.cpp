#include "UI/Storage/LSLobbyStorageWidget.h"

#include "Characters/LSPlayerCharacter.h"
#include "Components/TextBlock.h"
#include "Components/WrapBox.h"
#include "Core/LSPlayerControllerBase.h"
#include "Data/LSDropSettings.h"
#include "Data/LSItemRow.h"
#include "Engine/DataTable.h"
#include "Gameplay/LSWorldDroppedItem.h"
#include "Inventory/LSInventorySlotUtils.h"
#include "Layout/WidgetPath.h"
#include "LostSignal.h"
#include "Session/LSSaveSubsystem.h"
#include "UI/Inventory/LSInventoryDragDropOperation.h"
#include "UI/Inventory/LSItemSlotWidget.h"
#include "UI/Storage/LSStorageButtonWidget.h"

#define LOCTEXT_NAMESPACE "LSLobbyStorageWidget"

void ULSLobbyStorageWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!StorageSlotWrapBox)
	{
		UE_LOG(LogLS, Warning, TEXT("StorageSlotWrapBox is not bound on %s."), *GetNameSafe(this));
	}

	if (!StorageCountText)
	{
		UE_LOG(LogLS, Warning, TEXT("StorageCountText is not bound on %s."), *GetNameSafe(this));
	}

	if (!ItemSlotWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("ItemSlotWidgetClass is not set on %s."), *GetNameSafe(this));
	}

	BindStorageButtons();
	ApplyFilterButtonState();
	RefreshStorage();
}

void ULSLobbyStorageWidget::NativeDestruct()
{
	UnbindStorageButtons();

	Super::NativeDestruct();
}

void ULSLobbyStorageWidget::SetFilter(const ELSStorageFilter NewFilter)
{
	CurrentFilter = NewFilter;
	ApplyFilterButtonState();
	RefreshStorage();
}

void ULSLobbyStorageWidget::RefreshStorage()
{
	if (!StorageSlotWrapBox)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot refresh lobby storage because StorageSlotWrapBox is not bound on %s."), *GetNameSafe(this));
		return;
	}

	StorageSlotWrapBox->ClearChildren();

	ULSSaveSubsystem* SaveSubsystem = GetSaveSubsystem();
	static const TArray<FLSSessionItem> EmptyStashItems;
	const TArray<FLSSessionItem>& StashItems = SaveSubsystem ? SaveSubsystem->GetWarehouseItems() : EmptyStashItems;
	UpdateStorageCountText(StashItems);

	if (!SaveSubsystem)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot refresh lobby storage because SaveSubsystem is missing on %s."), *GetNameSafe(this));
		return;
	}

	if (!ItemSlotWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot refresh lobby storage because ItemSlotWidgetClass is not set on %s."), *GetNameSafe(this));
		return;
	}

	APlayerController* OwningPlayer = GetOwningPlayer();
	UWorld* World = GetWorld();
	if (!OwningPlayer && !World)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot refresh lobby storage because owner/world is missing on %s."), *GetNameSafe(this));
		return;
	}

	const int32 SlotCountToBuild = FMath::Max(0, MaxStorageSlotCount);

	if (CurrentFilter == ELSStorageFilter::All)
	{
		for (int32 VisualIndex = 0; VisualIndex < SlotCountToBuild; ++VisualIndex)
		{
			ULSItemSlotWidget* SlotWidget = OwningPlayer
				? CreateWidget<ULSItemSlotWidget>(OwningPlayer, ItemSlotWidgetClass)
				: CreateWidget<ULSItemSlotWidget>(World, ItemSlotWidgetClass);

			if (!SlotWidget)
			{
				UE_LOG(LogLS, Warning, TEXT("Failed to create lobby storage slot widget at index %d on %s."), VisualIndex, *GetNameSafe(this));
				continue;
			}

			const bool bHasItemAtSlot = StashItems.IsValidIndex(VisualIndex) && LSInventorySlotUtils::IsFilled(StashItems[VisualIndex]);
			if (bHasItemAtSlot)
			{
				SlotWidget->SetItem(StashItems[VisualIndex].ItemRowName, StashItems[VisualIndex].Amount, StashItems[VisualIndex].StatSeed);
			}
			else
			{
				SlotWidget->ClearItem();
			}

			SlotWidget->SetWarehouseSlotContext(this, ELSInventorySlotArea::Warehouse, VisualIndex, bHasItemAtSlot);
			StorageSlotWrapBox->AddChildToWrapBox(SlotWidget);
		}
	}
	else
	{
		TArray<TPair<int32, FLSSessionItem>> IndexedItems;
		BuildFilteredItems(StashItems, IndexedItems);

		if (IndexedItems.Num() > SlotCountToBuild)
		{
			UE_LOG(LogLS, Warning, TEXT("Lobby storage has %d filtered items but only %d slots are visible on %s."),
				IndexedItems.Num(),
				SlotCountToBuild,
				*GetNameSafe(this));
		}

		for (const TPair<int32, FLSSessionItem>& IndexedItem : IndexedItems)
		{
			ULSItemSlotWidget* SlotWidget = OwningPlayer
				? CreateWidget<ULSItemSlotWidget>(OwningPlayer, ItemSlotWidgetClass)
				: CreateWidget<ULSItemSlotWidget>(World, ItemSlotWidgetClass);

			if (!SlotWidget)
			{
				UE_LOG(LogLS, Warning, TEXT("Failed to create lobby storage slot widget for original index %d on %s."), IndexedItem.Key, *GetNameSafe(this));
				continue;
			}

			SlotWidget->SetItem(IndexedItem.Value.ItemRowName, IndexedItem.Value.Amount, IndexedItem.Value.StatSeed);
			SlotWidget->SetWarehouseSlotContext(this, ELSInventorySlotArea::Warehouse, IndexedItem.Key, true);
			StorageSlotWrapBox->AddChildToWrapBox(SlotWidget);
		}
	}
}

bool ULSLobbyStorageWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	ULSInventoryDragDropOperation* DragOperation = Cast<ULSInventoryDragDropOperation>(InOperation);
	if (!DragOperation || !DragOperation->SourceInventoryWidget || DragOperation->SourceSlotIndex == INDEX_NONE)
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	ULSSaveSubsystem* SaveSubsystem = GetSaveSubsystem();
	if (!SaveSubsystem)
	{
		return false;
	}

	const TArray<FLSSessionItem>& WarehouseItems = SaveSubsystem->GetWarehouseItems();
	int32 EmptyIndex = INDEX_NONE;
	for (int32 i = 0; i < WarehouseItems.Num(); ++i)
	{
		if (!LSInventorySlotUtils::IsFilled(WarehouseItems[i]))
		{
			EmptyIndex = i;
			break;
		}
	}

	if (EmptyIndex == INDEX_NONE)
	{
		EmptyIndex = WarehouseItems.Num();
	}

	return HandleStorageSlotDrop(DragOperation->SourceSlotArea, DragOperation->SourceSlotIndex, EmptyIndex);
}

bool ULSLobbyStorageWidget::HandleStorageSlotDrop(const ELSInventorySlotArea FromArea, const int32 FromIndex, const int32 ToWarehouseIndex)
{
	ULSSaveSubsystem* SaveSubsystem = GetSaveSubsystem();
	if (!SaveSubsystem)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot handle storage slot drop because SaveSubsystem is missing on %s."), *GetNameSafe(this));
		return false;
	}

	const bool bSuccess = SaveSubsystem->DropStoredSlot(FromArea, FromIndex, ELSInventorySlotArea::Warehouse, ToWarehouseIndex);
	if (bSuccess)
	{
		RefreshStorage();
		if (ALSPlayerCharacter* PlayerCharacter = Cast<ALSPlayerCharacter>(GetOwningPlayerPawn()))
		{
			PlayerCharacter->RebuildInventoryWidgetSlots();
		}
	}
	return bSuccess;
}

bool ULSLobbyStorageWidget::TryDropStorageDragToWorld(const ULSInventoryDragDropOperation& DragOperation, const FPointerEvent& PointerEvent)
{
	if (DragOperation.SourceLobbyStorageWidget != this)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot drop storage slot to world because source storage widget does not match."));
		return false;
	}

	if (DragOperation.SourceSlotIndex == INDEX_NONE)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot drop storage slot to world because source slot index is invalid."));
		return false;
	}

	if (IsPointerOverUserWidget(PointerEvent))
	{
		return false;
	}

	ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer());
	if (!PlayerController)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot drop storage slot to world because owning player controller is invalid on %s."), *GetNameSafe(this));
		return false;
	}

	FVector DropDirection = FVector::ZeroVector;
	PlayerController->ResolveDropDirectionFromSlatePosition(PointerEvent.GetScreenSpacePosition(), DropDirection);

	const bool bDropped = PlayerController->DropSessionSlotToWorld(
		DragOperation.SourceSlotArea,
		DragOperation.SourceSlotIndex,
		DroppedItemActorClass,
		DropDirection);

	if (bDropped)
	{
		RefreshStorage();
		if (ALSPlayerCharacter* PlayerCharacter = Cast<ALSPlayerCharacter>(GetOwningPlayerPawn()))
		{
			PlayerCharacter->RebuildInventoryWidgetSlots();
		}
	}

	return bDropped;
}

bool ULSLobbyStorageWidget::TransferStorageSlotToInventory(const int32 WarehouseSlotIndex)
{
	ALSPlayerCharacter* PlayerCharacter = Cast<ALSPlayerCharacter>(GetOwningPlayerPawn());
	if (!PlayerCharacter || !PlayerCharacter->IsInventoryWidgetOpen())
	{
		return false;
	}

	ULSSaveSubsystem* SaveSubsystem = GetSaveSubsystem();
	if (!SaveSubsystem)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot quick-transfer storage slot because SaveSubsystem is missing on %s."), *GetNameSafe(this));
		return false;
	}

	const bool bTransferred = SaveSubsystem->TransferStoredSlotToArea(
		ELSInventorySlotArea::Warehouse,
		WarehouseSlotIndex,
		ELSInventorySlotArea::Inventory);
	if (bTransferred)
	{
		RefreshStorage();
		PlayerCharacter->RebuildInventoryWidgetSlots();
	}

	return bTransferred;
}

void ULSLobbyStorageWidget::HandleSortButtonClicked()
{
	if (ULSSaveSubsystem* SaveSubsystem = GetSaveSubsystem())
	{
		SaveSubsystem->SortWarehouse();
		RefreshStorage();
		return;
	}

	UE_LOG(LogLS, Warning, TEXT("Cannot sort lobby storage because SaveSubsystem is missing on %s."), *GetNameSafe(this));
}

void ULSLobbyStorageWidget::HandleAllTabButtonClicked()
{
	SetFilter(ELSStorageFilter::All);
}

void ULSLobbyStorageWidget::HandleWeaponTabButtonClicked()
{
	SetFilter(ELSStorageFilter::Weapon);
}

void ULSLobbyStorageWidget::HandleArmorTabButtonClicked()
{
	SetFilter(ELSStorageFilter::Armor);
}

void ULSLobbyStorageWidget::HandleConsumableTabButtonClicked()
{
	SetFilter(ELSStorageFilter::Consumable);
}

void ULSLobbyStorageWidget::HandleMiscTabButtonClicked()
{
	SetFilter(ELSStorageFilter::Misc);
}

void ULSLobbyStorageWidget::HandleChipTabButtonClicked()
{
	SetFilter(ELSStorageFilter::Chip);
}

void ULSLobbyStorageWidget::BindStorageButtons()
{
	if (!SortButton)
	{
		UE_LOG(LogLS, Warning, TEXT("SortButton is not bound on %s."), *GetNameSafe(this));
	}
	else
	{
		SortButton->OnClicked.AddDynamic(this, &ULSLobbyStorageWidget::HandleSortButtonClicked);
	}

	if (!AllTabButton)
	{
		UE_LOG(LogLS, Warning, TEXT("AllTabButton is not bound on %s."), *GetNameSafe(this));
	}
	else
	{
		AllTabButton->OnClicked.AddDynamic(this, &ULSLobbyStorageWidget::HandleAllTabButtonClicked);
	}

	if (!WeaponTabButton)
	{
		UE_LOG(LogLS, Warning, TEXT("WeaponTabButton is not bound on %s."), *GetNameSafe(this));
	}
	else
	{
		WeaponTabButton->OnClicked.AddDynamic(this, &ULSLobbyStorageWidget::HandleWeaponTabButtonClicked);
	}

	if (!ArmorTabButton)
	{
		UE_LOG(LogLS, Warning, TEXT("ArmorTabButton is not bound on %s."), *GetNameSafe(this));
	}
	else
	{
		ArmorTabButton->OnClicked.AddDynamic(this, &ULSLobbyStorageWidget::HandleArmorTabButtonClicked);
	}

	if (!ConsumableTabButton)
	{
		UE_LOG(LogLS, Warning, TEXT("ConsumableTabButton is not bound on %s."), *GetNameSafe(this));
	}
	else
	{
		ConsumableTabButton->OnClicked.AddDynamic(this, &ULSLobbyStorageWidget::HandleConsumableTabButtonClicked);
	}

	if (!MiscTabButton)
	{
		UE_LOG(LogLS, Warning, TEXT("MiscTabButton is not bound on %s."), *GetNameSafe(this));
	}
	else
	{
		MiscTabButton->OnClicked.AddDynamic(this, &ULSLobbyStorageWidget::HandleMiscTabButtonClicked);
	}

	if (!ChipTabButton)
	{
		UE_LOG(LogLS, Warning, TEXT("ChipTabButton is not bound on %s."), *GetNameSafe(this));
	}
	else
	{
		ChipTabButton->OnClicked.AddDynamic(this, &ULSLobbyStorageWidget::HandleChipTabButtonClicked);
	}
}

void ULSLobbyStorageWidget::UnbindStorageButtons()
{
	if (SortButton)
	{
		SortButton->OnClicked.RemoveDynamic(this, &ULSLobbyStorageWidget::HandleSortButtonClicked);
	}

	if (AllTabButton)
	{
		AllTabButton->OnClicked.RemoveDynamic(this, &ULSLobbyStorageWidget::HandleAllTabButtonClicked);
	}

	if (WeaponTabButton)
	{
		WeaponTabButton->OnClicked.RemoveDynamic(this, &ULSLobbyStorageWidget::HandleWeaponTabButtonClicked);
	}

	if (ArmorTabButton)
	{
		ArmorTabButton->OnClicked.RemoveDynamic(this, &ULSLobbyStorageWidget::HandleArmorTabButtonClicked);
	}

	if (ConsumableTabButton)
	{
		ConsumableTabButton->OnClicked.RemoveDynamic(this, &ULSLobbyStorageWidget::HandleConsumableTabButtonClicked);
	}

	if (MiscTabButton)
	{
		MiscTabButton->OnClicked.RemoveDynamic(this, &ULSLobbyStorageWidget::HandleMiscTabButtonClicked);
	}

	if (ChipTabButton)
	{
		ChipTabButton->OnClicked.RemoveDynamic(this, &ULSLobbyStorageWidget::HandleChipTabButtonClicked);
	}
}

void ULSLobbyStorageWidget::UpdateStorageCountText(const TArray<FLSSessionItem>& StashItems) const
{
	if (!StorageCountText)
	{
		return;
	}

	int32 FilledSlotCount = 0;
	for (const FLSSessionItem& Item : StashItems)
	{
		if (LSInventorySlotUtils::IsFilled(Item))
		{
			++FilledSlotCount;
		}
	}

	StorageCountText->SetText(FText::Format(
		LOCTEXT("StorageCountFormat", "{0}/{1}"),
		FText::AsNumber(FilledSlotCount),
		FText::AsNumber(FMath::Max(0, MaxStorageSlotCount))));
}

void ULSLobbyStorageWidget::ApplyFilterButtonState() const
{
	if (AllTabButton)
	{
		AllTabButton->SetIsEnabled(CurrentFilter != ELSStorageFilter::All);
	}

	if (WeaponTabButton)
	{
		WeaponTabButton->SetIsEnabled(CurrentFilter != ELSStorageFilter::Weapon);
	}

	if (ArmorTabButton)
	{
		ArmorTabButton->SetIsEnabled(CurrentFilter != ELSStorageFilter::Armor);
	}

	if (ConsumableTabButton)
	{
		ConsumableTabButton->SetIsEnabled(CurrentFilter != ELSStorageFilter::Consumable);
	}

	if (MiscTabButton)
	{
		MiscTabButton->SetIsEnabled(CurrentFilter != ELSStorageFilter::Misc);
	}

	if (ChipTabButton)
	{
		ChipTabButton->SetIsEnabled(CurrentFilter != ELSStorageFilter::Chip);
	}
}

void ULSLobbyStorageWidget::BuildFilteredItems(const TArray<FLSSessionItem>& StashItems, TArray<TPair<int32, FLSSessionItem>>& OutIndexedItems) const
{
	OutIndexedItems.Reset();

	for (int32 i = 0; i < StashItems.Num(); ++i)
	{
		const FLSSessionItem& Item = StashItems[i];
		if (LSInventorySlotUtils::IsFilled(Item) && DoesItemMatchCurrentFilter(Item.ItemRowName))
		{
			OutIndexedItems.Add(TPair<int32, FLSSessionItem>(i, Item));
		}
	}
}

bool ULSLobbyStorageWidget::DoesItemMatchCurrentFilter(const FName ItemRowName) const
{
	if (ItemRowName.IsNone())
	{
		return false;
	}

	const FString RowNameString = ItemRowName.ToString();
	switch (CurrentFilter)
	{
	case ELSStorageFilter::All:
		return true;
	case ELSStorageFilter::Weapon:
		return RowNameString.StartsWith(TEXT("Weapon_"));
	case ELSStorageFilter::Armor:
		return RowNameString.StartsWith(TEXT("Armor_"));
	case ELSStorageFilter::Consumable:
		return IsConsumableItem(ItemRowName);
	case ELSStorageFilter::Misc:
		return RowNameString.StartsWith(TEXT("Item_")) && !IsConsumableItem(ItemRowName);
	case ELSStorageFilter::Chip:
		return RowNameString.StartsWith(TEXT("Chip_"));
	default:
		return false;
	}
}

bool ULSLobbyStorageWidget::IsConsumableItem(const FName ItemRowName) const
{
	const FString RowNameString = ItemRowName.ToString();
	if (!RowNameString.StartsWith(TEXT("Item_")))
	{
		return false;
	}

	const ULSDropSettings* Settings = GetDefault<ULSDropSettings>();
	if (!Settings)
	{
		return false;
	}

	UDataTable* ItemTable = Settings->ItemTable.LoadSynchronous();
	const FLSItemRow* Row = ItemTable ? ItemTable->FindRow<FLSItemRow>(ItemRowName, TEXT("LSLobbyStorageFilter")) : nullptr;
	if (!Row)
	{
		return false;
	}

	return Row->Item_Type >= 4 && Row->Item_Type <= 9;
}

bool ULSLobbyStorageWidget::IsPointerOverUserWidget(const FPointerEvent& PointerEvent) const
{
	const FWidgetPath* EventPath = PointerEvent.GetEventPath();
	if (!EventPath)
	{
		return false;
	}

	static const FName ObjectWidgetTypeName(TEXT("SObjectWidget"));
	for (int32 WidgetIndex = 0; WidgetIndex < EventPath->Widgets.Num(); ++WidgetIndex)
	{
		const TSharedRef<SWidget>& Widget = EventPath->Widgets[WidgetIndex].Widget;
		if (Widget->GetType() == ObjectWidgetTypeName)
		{
			return true;
		}
	}

	return false;
}

ULSSaveSubsystem* ULSLobbyStorageWidget::GetSaveSubsystem() const
{
	UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
}

#undef LOCTEXT_NAMESPACE
