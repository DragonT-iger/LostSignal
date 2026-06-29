#include "UI/Inventory/LSInventoryWidget.h"

#include "Components/Border.h"
#include "LostSignal.h"
#include "Components/Button.h"
#include "Components/WrapBox.h"
#include "Core/LSPlayerControllerBase.h"
#include "Gameplay/LSWorldDroppedItem.h"
#include "Inventory/LSRaidInventoryComponent.h"
#include "Layout/WidgetPath.h"
#include "Session/LSSaveSubsystem.h"
#include "UI/Inventory/LSInventoryDragDropOperation.h"
#include "UI/Inventory/LSItemSlotWidget.h"
#include "UI/Inventory/LSSlotWidgetSync.h"
#include "UI/LootDrop/LSLootDropWidget.h"

namespace
{
void AppendSlotItems(TArray<FLSSessionItem>& Items, const TArray<FLSSessionItem>& NewItems)
{
	for (const FLSSessionItem& NewItem : NewItems)
	{
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
		SetStoreAllButtonVisible(false);
	}

	if (!SortButton)
	{
		UE_LOG(LogLS, Warning, TEXT("SortButton is not bound on %s."), *GetNameSafe(this));
	}
	else
	{
		SortButton->OnClicked.AddDynamic(this, &ULSInventoryWidget::HandleSortButtonClicked);
	}

	InitializeDisplayOnlyEquipmentSlot(WeaponSlot, TEXT("WeaponSlot"));
	InitializeDisplayOnlyEquipmentSlot(HeadphoneSlot, TEXT("HeadphoneSlot"));
	InitializeDisplayOnlyEquipmentSlot(HeadSlot, TEXT("HeadSlot"));
	InitializeDisplayOnlyEquipmentSlot(GlovesSlot, TEXT("GlovesSlot"));
	InitializeDisplayOnlyEquipmentSlot(BodySlot, TEXT("BodySlot"));

	RebuildInventorySlots();
	RebuildConfirmedStorageSlots();

	// 폰 없는 로비에서도 창고↔인벤토리 갱신이 동작하도록 PC에 자신을 등록한다(폰이 있으면 PC가 폰을 우선 사용).
	if (ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer()))
	{
		PlayerController->RegisterLobbyInventoryWidget(this);
	}
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

	if (ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer()))
	{
		PlayerController->UnregisterLobbyInventoryWidget(this);
	}

	Super::NativeDestruct();
}

bool ULSInventoryWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	if (HandleInventoryBackgroundDrop(InGeometry, InDragDropEvent, InOperation))
	{
		return true;
	}

	return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
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

	if (!ItemSlotWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("ItemSlotWidgetClass is not set on %s."), *GetNameSafe(this));
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
	int32 SlotCountToBuild = InventorySlotCount;
	bool bUsingRaidInventory = false;
	if (ALSPlayerControllerBase* LSPlayerController = Cast<ALSPlayerControllerBase>(OwningPlayer))
	{
		if (ULSRaidInventoryComponent* RaidInventory = LSPlayerController->GetRaidInventoryComponent())
		{
			if (RaidInventory->IsRaidActive())
			{
				AppendSlotItems(InventoryItems, RaidInventory->GetSessionInventory());
				SlotCountToBuild = RaidInventory->GetMaxInventorySlotCount();
				bUsingRaidInventory = true;
			}
		}
	}

	if (!bUsingRaidInventory)
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (ULSSaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<ULSSaveSubsystem>())
			{
				AppendSlotItems(InventoryItems, SaveSubsystem->GetInventory());
				SlotCountToBuild = SaveSubsystem->GetMaxInventorySlotCount();
			}
			else
			{
				UE_LOG(LogLS, Warning, TEXT("SaveSubsystem is missing on %s."), *GetNameSafe(this));
			}
		}
	}

	UE_LOG(LogLS, Log, TEXT("InventoryWidget rebuilt with %d slot items on %s."), InventoryItems.Num(), *GetNameSafe(this));

	LSSlotWidgetSync::SyncSlotWidgets(InventoryWrapBox, ItemSlotWidgetClass, OwningPlayer, World, SlotCountToBuild,
		[this, &InventoryItems](const int32 SlotIndex, ULSItemSlotWidget& SlotWidget)
		{
			const bool bHasSlotItem = InventoryItems.IsValidIndex(SlotIndex) &&
				!InventoryItems[SlotIndex].ItemRowName.IsNone() &&
				InventoryItems[SlotIndex].Amount > 0;
			SlotWidget.SetSlotContext(this, ELSInventorySlotArea::Inventory, SlotIndex, bHasSlotItem);

			if (bHasSlotItem)
			{
				SlotWidget.SetItem(InventoryItems[SlotIndex].ItemRowName, InventoryItems[SlotIndex].Amount, InventoryItems[SlotIndex].ChipStats);
			}
			else
			{
				SlotWidget.ClearItem();
			}
		});
}

bool ULSInventoryWidget::HandleInventorySlotDrop(const ELSInventorySlotArea FromSlotArea, const int32 FromSlotIndex, const ELSInventorySlotArea ToSlotArea, const int32 ToSlotIndex)
{
	if (FromSlotIndex == INDEX_NONE || ToSlotIndex == INDEX_NONE)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot handle inventory slot drop because an index is invalid. From=%d To=%d"), FromSlotIndex, ToSlotIndex);
		return false;
	}

	if (IsSlotLocked(FromSlotArea, FromSlotIndex) || IsSlotLocked(ToSlotArea, ToSlotIndex))
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot handle inventory slot drop because a slot is locked. FromArea=%d From=%d ToArea=%d To=%d"),
			static_cast<int32>(FromSlotArea),
			FromSlotIndex,
			static_cast<int32>(ToSlotArea),
			ToSlotIndex);
		return false;
	}

	ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer());
	if (!PlayerController)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot handle inventory slot drop because owning player controller is invalid on %s."), *GetNameSafe(this));
		return false;
	}

	ULSRaidInventoryComponent* RaidInventory = PlayerController->GetRaidInventoryComponent();
	if (!RaidInventory || !RaidInventory->IsRaidActive())
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (ULSSaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<ULSSaveSubsystem>())
			{
				const bool bChanged = SaveSubsystem->DropStoredSlot(FromSlotArea, FromSlotIndex, ToSlotArea, ToSlotIndex);
				if (bChanged)
				{
					RebuildInventorySlots();
					RebuildConfirmedStorageSlots();
					// 창고↔인벤토리 이동이면 열려 있는 창고 위젯도 같이 갱신한다.
					PlayerController->RefreshOpenLobbyStorageWidget();
				}
				return bChanged;
			}
		}

		UE_LOG(LogLS, Warning, TEXT("Cannot handle stored inventory slot drop because SaveSubsystem is missing on %s."), *GetNameSafe(this));
		return false;
	}

	const bool bChanged = PlayerController->DropInventorySlot(FromSlotArea, FromSlotIndex, ToSlotArea, ToSlotIndex);

	UE_LOG(LogLS, Log, TEXT("Inventory slot drop handled on %s. From=%d To=%d Changed=%s"),
		*GetNameSafe(this),
		FromSlotIndex,
		ToSlotIndex,
		bChanged ? TEXT("true") : TEXT("false"));

	if (bChanged && PlayerController->HasAuthority())
	{
		RebuildInventorySlots();
		RebuildConfirmedStorageSlots();
	}

	return bChanged;
}

bool ULSInventoryWidget::HandleLootSlotDrop(ULSLootDropWidget* LootDropWidget, const int32 LootSlotIndex, const ELSInventorySlotArea ToSlotArea, const int32 ToSlotIndex)
{
	if (!LootDropWidget)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot handle loot slot drop because LootDropWidget is missing on %s."), *GetNameSafe(this));
		return false;
	}

	const bool bTransferred = LootDropWidget->TransferLootSlotToInventorySlot(LootSlotIndex, ToSlotArea, ToSlotIndex);
	APlayerController* OwningPlayer = GetOwningPlayer();
	if (bTransferred && OwningPlayer && OwningPlayer->HasAuthority())
	{
		RebuildInventorySlots();
		RebuildConfirmedStorageSlots();
	}

	return bTransferred;
}

bool ULSInventoryWidget::TryDropInventoryDragToWorld(const ULSInventoryDragDropOperation& DragOperation, const FPointerEvent& PointerEvent)
{
	if (DragOperation.SourceLootDropWidget)
	{
		return false;
	}

	if (DragOperation.SourceInventoryWidget != this)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot drop inventory slot to world because source inventory widget does not match."));
		return false;
	}

	if (DragOperation.SourceSlotIndex == INDEX_NONE)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot drop inventory slot to world because source slot index is invalid."));
		return false;
	}

	if (IsSlotLocked(DragOperation.SourceSlotArea, DragOperation.SourceSlotIndex))
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot drop inventory slot to world because source slot is locked. Area=%d Index=%d"),
			static_cast<int32>(DragOperation.SourceSlotArea),
			DragOperation.SourceSlotIndex);
		return false;
	}

	if (!InventoryWindowBorder)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot drop inventory slot to world because InventoryWindowBorder is not bound on %s."), *GetNameSafe(this));
		return false;
	}

	const FVector2D ScreenPosition = PointerEvent.GetScreenSpacePosition();
	if (IsPointerInsideInventoryWindow(ScreenPosition) || IsPointerOverUserWidget(PointerEvent))
	{
		return false;
	}

	return DropInventoryDragToWorld(DragOperation, ScreenPosition);
}

void ULSInventoryWidget::RebuildConfirmedStorageSlots()
{
	if (!ConfirmedStorageSlotWrapBox)
	{
		UE_LOG(LogLS, Warning, TEXT("ConfirmedStorageSlotWrapBox is not bound on %s."), *GetNameSafe(this));
		return;
	}

	if (!ItemSlotWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("ItemSlotWidgetClass is not set on %s. Confirmed storage slots use the same widget class."), *GetNameSafe(this));
		return;
	}

	APlayerController* OwningPlayer = GetOwningPlayer();
	UWorld* World = GetWorld();
	if (!OwningPlayer && !World)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot rebuild confirmed storage slots because owner/world is missing on %s."), *GetNameSafe(this));
		return;
	}

	TArray<FLSSessionItem> SafeItems;
	int32 SlotCountToBuild = ConfirmedStorageSlotCount;
	bool bUsingRaidInventory = false;
	if (ALSPlayerControllerBase* LSPlayerController = Cast<ALSPlayerControllerBase>(OwningPlayer))
	{
		if (ULSRaidInventoryComponent* RaidInventory = LSPlayerController->GetRaidInventoryComponent())
		{
			if (RaidInventory->IsRaidActive())
			{
				AppendSlotItems(SafeItems, RaidInventory->GetSessionSafeInventory());
				SlotCountToBuild = FMath::Max(RaidInventory->GetMaxSafeSlotCount(), SafeItems.Num());
				bUsingRaidInventory = true;
			}
		}
	}

	if (!bUsingRaidInventory)
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (ULSSaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<ULSSaveSubsystem>())
			{
				AppendSlotItems(SafeItems, SaveSubsystem->GetSafeStash());
				SlotCountToBuild = FMath::Max(SaveSubsystem->GetMaxSafeStashSlotCount(), SafeItems.Num());
			}
		}
	}

	LSSlotWidgetSync::SyncSlotWidgets(ConfirmedStorageSlotWrapBox, ItemSlotWidgetClass, OwningPlayer, World, SlotCountToBuild,
		[this, &SafeItems](const int32 SlotIndex, ULSItemSlotWidget& SlotWidget)
		{
			const bool bHasSlotItem = SafeItems.IsValidIndex(SlotIndex) &&
				!SafeItems[SlotIndex].ItemRowName.IsNone() &&
				SafeItems[SlotIndex].Amount > 0;
			const bool bIsLocked = IsSlotLocked(ELSInventorySlotArea::Safe, SlotIndex);
			SlotWidget.SetSlotContext(this, ELSInventorySlotArea::Safe, SlotIndex, bHasSlotItem, bIsLocked);
			if (bHasSlotItem)
			{
				SlotWidget.SetItem(SafeItems[SlotIndex].ItemRowName, SafeItems[SlotIndex].Amount, SafeItems[SlotIndex].ChipStats);
			}
			else
			{
				SlotWidget.ClearItem();
			}
		});
}

void ULSInventoryWidget::SetStoreAllButtonVisible(const bool bVisible)
{
	if (!StoreAllButton)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot update StoreAllButton visibility because StoreAllButton is not bound on %s."), *GetNameSafe(this));
		return;
	}

	StoreAllButton->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
}

void ULSInventoryWidget::HandleStoreAllButtonClicked()
{
	ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer());
	if (!PlayerController)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot store all inventory items because owning player controller is invalid on %s."), *GetNameSafe(this));
		return;
	}

	if (ULSRaidInventoryComponent* RaidInventory = PlayerController->GetRaidInventoryComponent())
	{
		if (RaidInventory->IsRaidActive())
		{
			UE_LOG(LogLS, Warning, TEXT("Cannot store all inventory items during a raid on %s."), *GetNameSafe(this));
			return;
		}
	}

	if (!PlayerController->IsLobbyStorageWidgetOpen())
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot store all inventory items because lobby storage is not open on %s."), *GetNameSafe(this));
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	ULSSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
	if (!SaveSubsystem)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot store all inventory items because SaveSubsystem is missing on %s."), *GetNameSafe(this));
		return;
	}

	bool bStoppedBecauseFull = false;
	const bool bChanged = SaveSubsystem->TransferAllInventoryToWarehouse(
		PlayerController->GetOpenLobbyStorageMaxSlotCount(),
		bStoppedBecauseFull);

	if (bChanged)
	{
		RebuildInventorySlots();
		RebuildConfirmedStorageSlots();
		PlayerController->RefreshOpenLobbyStorageWidget();
	}

	if (bStoppedBecauseFull)
	{
		UE_LOG(LogLS, Warning, TEXT("Store all inventory items stopped because lobby storage is full on %s."), *GetNameSafe(this));
	}
}

void ULSInventoryWidget::HandleSortButtonClicked()
{
	if (ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer()))
	{
		if (ULSRaidInventoryComponent* RaidInventory = PlayerController->GetRaidInventoryComponent())
		{
			if (RaidInventory->IsRaidActive())
			{
				if (PlayerController->SortRaidInventory() && PlayerController->HasAuthority())
				{
					RebuildInventorySlots();
					RebuildConfirmedStorageSlots();
				}
				return;
			}
		}
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (ULSSaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<ULSSaveSubsystem>())
		{
			SaveSubsystem->SortInventory();
		}
		else
		{
			UE_LOG(LogLS, Warning, TEXT("Cannot sort player inventory because SaveSubsystem is missing on %s."), *GetNameSafe(this));
		}
	}

	RebuildInventorySlots();
	RebuildConfirmedStorageSlots();
}

void ULSInventoryWidget::InitializeDisplayOnlyEquipmentSlot(ULSItemSlotWidget* SlotWidget, const TCHAR* SlotName) const
{
	if (!SlotWidget)
	{
		UE_LOG(LogLS, Warning, TEXT("%s is not bound on %s."), SlotName, *GetNameSafe(this));
		return;
	}

	SlotWidget->SetDisplayOnlySlotContext();
	SlotWidget->ClearItem();
}

bool ULSInventoryWidget::HandleInventoryBackgroundDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	ULSInventoryDragDropOperation* DragOperation = Cast<ULSInventoryDragDropOperation>(InOperation);
	if (!DragOperation)
	{
		return false;
	}

	if (DragOperation->SourceLootDropWidget)
	{
		return false;
	}

	if (IsPointerInsideInventoryWindow(InDragDropEvent.GetScreenSpacePosition()))
	{
		return false;
	}

	return DropInventoryDragToWorld(*DragOperation, InDragDropEvent.GetScreenSpacePosition());
}

bool ULSInventoryWidget::DropInventoryDragToWorld(const ULSInventoryDragDropOperation& DragOperation, const FVector2D ScreenPosition)
{
	if (DragOperation.SourceInventoryWidget != this)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot drop inventory slot to world because source inventory widget does not match."));
		return false;
	}

	if (DragOperation.SourceSlotIndex == INDEX_NONE)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot drop inventory slot to world because source slot index is invalid."));
		return false;
	}

	if (IsSlotLocked(DragOperation.SourceSlotArea, DragOperation.SourceSlotIndex))
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot drop inventory slot to world because source slot is locked. Area=%d Index=%d"),
			static_cast<int32>(DragOperation.SourceSlotArea),
			DragOperation.SourceSlotIndex);
		return false;
	}

	ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer());
	if (!PlayerController)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot drop inventory slot to world because owning player controller is invalid on %s."), *GetNameSafe(this));
		return false;
	}

	if (!DroppedItemActorClass)
	{
		UE_LOG(LogLS, Warning, TEXT("DroppedItemActorClass is not set on %s. Dropped item will use native class and may not show interact hint UI."), *GetNameSafe(this));
	}

	FVector DropDirection = FVector::ZeroVector;
	PlayerController->ResolveDropDirectionFromSlatePosition(ScreenPosition, DropDirection);

	const bool bDropped = PlayerController->DropSessionSlotToWorld(
		DragOperation.SourceSlotArea,
		DragOperation.SourceSlotIndex,
		DroppedItemActorClass,
		DropDirection);

	if (bDropped)
	{
		RebuildInventorySlots();
		RebuildConfirmedStorageSlots();
	}

	return bDropped;
}

bool ULSInventoryWidget::IsSlotLocked(const ELSInventorySlotArea SlotArea, const int32 SlotIndex) const
{
	if (SlotArea != ELSInventorySlotArea::Safe || SlotIndex < 0)
	{
		return false;
	}

	APlayerController* OwningPlayer = GetOwningPlayer();
	if (ALSPlayerControllerBase* LSPlayerController = Cast<ALSPlayerControllerBase>(OwningPlayer))
	{
		if (ULSRaidInventoryComponent* RaidInventory = LSPlayerController->GetRaidInventoryComponent())
		{
			if (RaidInventory->IsRaidActive())
			{
				return SlotIndex >= RaidInventory->GetMaxSafeSlotCount();
			}
		}
	}

	UGameInstance* GameInstance = GetGameInstance();
	const ULSSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
	return SaveSubsystem && SlotIndex >= SaveSubsystem->GetMaxSafeStashSlotCount();
}

bool ULSInventoryWidget::IsPointerInsideInventoryWindow(const FVector2D ScreenPosition) const
{
	if (!InventoryWindowBorder)
	{
		return false;
	}

	const FGeometry& InventoryWindowGeometry = InventoryWindowBorder->GetCachedGeometry();
	const FVector2D LocalDropPosition = InventoryWindowGeometry.AbsoluteToLocal(ScreenPosition);
	return LocalDropPosition.X >= 0.0f &&
		LocalDropPosition.Y >= 0.0f &&
		LocalDropPosition.X <= InventoryWindowGeometry.GetLocalSize().X &&
		LocalDropPosition.Y <= InventoryWindowGeometry.GetLocalSize().Y;
}

bool ULSInventoryWidget::IsPointerOverUserWidget(const FPointerEvent& PointerEvent) const
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
