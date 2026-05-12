#include "UI/Inventory/LSInventoryWidget.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/Border.h"
#include "Components/CapsuleComponent.h"
#include "LostSignal.h"
#include "Components/Button.h"
#include "Components/WrapBox.h"
#include "Core/LSPlayerControllerBase.h"
#include "Session/LSSaveSubsystem.h"
#include "Session/LSSessionSubsystem.h"
#include "UI/Inventory/LSInventoryDragDropOperation.h"
#include "UI/Inventory/LSInventoryItemSlotWidget.h"

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
			const bool bHasSlotItem = InventoryItems.IsValidIndex(SlotIndex) &&
				!InventoryItems[SlotIndex].ItemRowName.IsNone() &&
				InventoryItems[SlotIndex].Amount > 0;
			SlotWidget->SetSlotContext(this, ELSInventorySlotArea::Inventory, SlotIndex, bHasSlotItem);

			if (bHasSlotItem)
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

bool ULSInventoryWidget::HandleInventorySlotDrop(const ELSInventorySlotArea FromSlotArea, const int32 FromSlotIndex, const ELSInventorySlotArea ToSlotArea, const int32 ToSlotIndex)
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

	const bool bChanged = SessionSubsystem->DropSessionSlot(FromSlotArea, FromSlotIndex, ToSlotArea, ToSlotIndex);

	UE_LOG(LogLS, Log, TEXT("Inventory slot drop handled on %s. From=%d To=%d Changed=%s"),
		*GetNameSafe(this),
		FromSlotIndex,
		ToSlotIndex,
		bChanged ? TEXT("true") : TEXT("false"));

	if (bChanged)
	{
		RebuildInventorySlots();
		RebuildConfirmedStorageSlots();
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

	TArray<FLSSessionItem> SafeItems;
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (ULSSessionSubsystem* SessionSubsystem = GameInstance->GetSubsystem<ULSSessionSubsystem>())
		{
			if (SessionSubsystem->IsRaidActive())
			{
				AppendSlotItems(SafeItems, SessionSubsystem->GetSessionSafeInventory());
			}
			else if (ULSSaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<ULSSaveSubsystem>())
			{
				AppendSlotItems(SafeItems, SaveSubsystem->GetSafeStash());
			}
		}
	}

	const int32 SlotCountToBuild = FMath::Max(ConfirmedStorageSlotCount, SafeItems.Num());
	for (int32 SlotIndex = 0; SlotIndex < SlotCountToBuild; ++SlotIndex)
	{
		ULSInventoryItemSlotWidget* SlotWidget = OwningPlayer
			? CreateWidget<ULSInventoryItemSlotWidget>(OwningPlayer, InventoryItemSlotWidgetClass)
			: CreateWidget<ULSInventoryItemSlotWidget>(World, InventoryItemSlotWidgetClass);

		if (SlotWidget)
		{
			const bool bHasSlotItem = SafeItems.IsValidIndex(SlotIndex) &&
				!SafeItems[SlotIndex].ItemRowName.IsNone() &&
				SafeItems[SlotIndex].Amount > 0;
			SlotWidget->SetSlotContext(this, ELSInventorySlotArea::Safe, SlotIndex, bHasSlotItem);
			if (bHasSlotItem)
			{
				SlotWidget->SetItem(SafeItems[SlotIndex].ItemRowName, SafeItems[SlotIndex].Amount);
			}
			else
			{
				SlotWidget->ClearItem();
			}
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

bool ULSInventoryWidget::HandleInventoryBackgroundDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	ULSInventoryDragDropOperation* DragOperation = Cast<ULSInventoryDragDropOperation>(InOperation);
	if (!DragOperation)
	{
		return false;
	}

	if (DragOperation->SourceInventoryWidget != this)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot drop inventory slot to world because source inventory widget does not match."));
		return false;
	}

	if (DragOperation->SourceSlotIndex == INDEX_NONE)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot drop inventory slot to world because source slot index is invalid."));
		return false;
	}

	if (!InventoryWindowBorder)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot drop inventory slot to world because InventoryWindowBorder is not bound on %s."), *GetNameSafe(this));
		return false;
	}

	const FGeometry& InventoryWindowGeometry = InventoryWindowBorder->GetCachedGeometry();
	const FVector2D LocalDropPosition = InventoryWindowGeometry.AbsoluteToLocal(InDragDropEvent.GetScreenSpacePosition());
	if (LocalDropPosition.X >= 0.0f &&
		LocalDropPosition.Y >= 0.0f &&
		LocalDropPosition.X <= InventoryWindowGeometry.GetLocalSize().X &&
		LocalDropPosition.Y <= InventoryWindowGeometry.GetLocalSize().Y)
	{
		return false;
	}

	FVector DropLocation = FVector::ZeroVector;
	if (!ResolveDroppedItemLocation(DropLocation))
	{
		return false;
	}

	float DropYaw = 0.0f;
	if (!ResolveDroppedItemYaw(DropYaw))
	{
		return false;
	}

	ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer());
	if (!PlayerController)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot drop inventory slot to world because owning player controller is invalid on %s."), *GetNameSafe(this));
		return false;
	}

	const bool bDropped = PlayerController->DropSessionSlotToWorld(
		DragOperation->SourceSlotArea,
		DragOperation->SourceSlotIndex,
		DroppedItemActorClass,
		DropLocation,
		DropYaw);

	if (bDropped)
	{
		RebuildInventorySlots();
		RebuildConfirmedStorageSlots();
	}

	return bDropped;
}

bool ULSInventoryWidget::ResolveDroppedItemLocation(FVector& OutDropLocation) const
{
	const APlayerController* PlayerController = GetOwningPlayer();
	const APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!Pawn)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot resolve dropped item location because owning pawn is missing on %s."), *GetNameSafe(this));
		return false;
	}

	OutDropLocation = Pawn->GetActorLocation();
	if (const UCapsuleComponent* CapsuleComponent = Pawn->FindComponentByClass<UCapsuleComponent>())
	{
		OutDropLocation.Z -= CapsuleComponent->GetScaledCapsuleHalfHeight();
	}

	constexpr float DropZRandomRange = 3.0f;
	OutDropLocation.Z += FMath::FRandRange(0.0f, DropZRandomRange);
	return true;
}

bool ULSInventoryWidget::ResolveDroppedItemYaw(float& OutDropYaw) const
{
	const APlayerController* PlayerController = GetOwningPlayer();
	if (!PlayerController || !PlayerController->PlayerCameraManager)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot resolve dropped item yaw because player camera manager is missing on %s."), *GetNameSafe(this));
		return false;
	}

	OutDropYaw = PlayerController->PlayerCameraManager->GetCameraRotation().Yaw + 180.0f;
	return true;
}
