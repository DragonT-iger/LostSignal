#include "UI/Inventory/LSInventoryWidget.h"

#include "Components/Border.h"
#include "LostSignal.h"
#include "Components/Button.h"
#include "Components/WrapBox.h"
#include "Core/LSPlayerControllerBase.h"
#include "Gameplay/LSWorldDroppedItem.h"
#include "Inventory/LSInventorySlotUtils.h"
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

	RebuildInventorySlots();
	RebuildConfirmedStorageSlots();
	RebuildEquipmentSlots();

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

	// 장비 슬롯이 원본/대상에 걸리면 로비 전용 장비 이동 경로로 처리한다(레이드 라우팅 우회).
	if (FromSlotArea == ELSInventorySlotArea::Equipment || ToSlotArea == ELSInventorySlotArea::Equipment)
	{
		return HandleEquipmentSlotDrop(FromSlotArea, FromSlotIndex, ToSlotArea, ToSlotIndex);
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
					// 칩이 옮겨졌으면 열려 있는 칩 스테이션 칩 리스트도 다시 그린다(stale 방지).
					PlayerController->RefreshOpenChipStationWidget();
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

bool ULSInventoryWidget::HandleEquipmentSlotDrop(const ELSInventorySlotArea FromSlotArea, const int32 FromSlotIndex, const ELSInventorySlotArea ToSlotArea, const int32 ToSlotIndex)
{
	if (FromSlotIndex == INDEX_NONE || ToSlotIndex == INDEX_NONE)
	{
		return false;
	}

	// 장비 장착/해제는 로비 전용이다. 레이드 중에는 인벤토리 원본이 SaveSubsystem이 아니라 세션 상태라 변경을 막는다.
	if (ALSPlayerControllerBase* LSPlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer()))
	{
		if (ULSRaidInventoryComponent* RaidInventory = LSPlayerController->GetRaidInventoryComponent())
		{
			if (RaidInventory->IsRaidActive())
			{
				UE_LOG(LogLS, Warning, TEXT("Cannot change equipment during a raid on %s."), *GetNameSafe(this));
				return false;
			}
		}
	}

	// 잠긴 보호 슬롯(적재 프로토콜 감소분 등)은 장비 이동 원본/대상으로 쓸 수 없다.
	if (IsSlotLocked(FromSlotArea, FromSlotIndex) || IsSlotLocked(ToSlotArea, ToSlotIndex))
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot handle equipment slot drop because a slot is locked. FromArea=%d From=%d ToArea=%d To=%d"),
			static_cast<int32>(FromSlotArea), FromSlotIndex, static_cast<int32>(ToSlotArea), ToSlotIndex);
		return false;
	}

	UGameInstance* GameInstance = GetGameInstance();
	ULSSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
	if (!SaveSubsystem)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot handle equipment slot drop because SaveSubsystem is missing on %s."), *GetNameSafe(this));
		return false;
	}

	const bool bChanged = SaveSubsystem->MoveEquipmentSlot(FromSlotArea, FromSlotIndex, ToSlotArea, ToSlotIndex);
	if (bChanged)
	{
		RebuildInventorySlots();
		RebuildConfirmedStorageSlots();
		RebuildEquipmentSlots();
		// 장비를 인벤토리에서 뺐거나 넣었으면 열려 있는 창고 위젯도 갱신(인벤토리 인덱스 변화 반영).
		if (ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer()))
		{
			PlayerController->RefreshOpenLobbyStorageWidget();
			PlayerController->RefreshOpenChipStationWidget();
		}
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

	// 장착된 장비는 창 밖으로 드래그해도 월드에 버리지 않는다(드래그 취소 시 장착 유지).
	if (DragOperation.SourceSlotArea == ELSInventorySlotArea::Equipment)
	{
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
		PlayerController->RefreshOpenChipStationWidget();
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
	// 정렬은 인벤토리를 압축해 인덱스를 재배치하므로, 열려 있는 칩 스테이션 칩 리스트도 다시 그린다.
	if (ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer()))
	{
		PlayerController->RefreshOpenChipStationWidget();
	}
}

void ULSInventoryWidget::RebuildEquipmentSlots()
{
	// 장비 장착은 로비 전용이다. 레이드 중에는 표시만 유지하고 잠금 처리해 드래그/변경을 막는다.
	bool bRaidActive = false;
	if (ALSPlayerControllerBase* LSPlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer()))
	{
		if (ULSRaidInventoryComponent* RaidInventory = LSPlayerController->GetRaidInventoryComponent())
		{
			bRaidActive = RaidInventory->IsRaidActive();
		}
	}

	UGameInstance* GameInstance = GetGameInstance();
	ULSSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
	static const TArray<FLSSessionItem> EmptyEquipment;
	const TArray<FLSSessionItem>& EquipmentItems = SaveSubsystem ? SaveSubsystem->GetEquipmentSlots() : EmptyEquipment;

	// ELSEquipmentSlot 순서와 일치해야 한다(인덱스 = 슬롯 타입).
	ULSItemSlotWidget* SlotWidgets[] = { WeaponSlot, ProcessorSlot, CoreSlot, ActuatorSlot, FrameSlot };
	const TCHAR* SlotNames[] = { TEXT("WeaponSlot"), TEXT("ProcessorSlot"), TEXT("CoreSlot"), TEXT("ActuatorSlot"), TEXT("FrameSlot") };

	for (int32 SlotIndex = 0; SlotIndex < UE_ARRAY_COUNT(SlotWidgets); ++SlotIndex)
	{
		ULSItemSlotWidget* SlotWidget = SlotWidgets[SlotIndex];
		if (!SlotWidget)
		{
			UE_LOG(LogLS, Warning, TEXT("%s is not bound on %s."), SlotNames[SlotIndex], *GetNameSafe(this));
			continue;
		}

		const bool bHasSlotItem = EquipmentItems.IsValidIndex(SlotIndex) &&
			!EquipmentItems[SlotIndex].ItemRowName.IsNone() &&
			EquipmentItems[SlotIndex].Amount > 0;
		SlotWidget->SetSlotContext(this, ELSInventorySlotArea::Equipment, SlotIndex, bHasSlotItem, bRaidActive);
		if (bHasSlotItem)
		{
			SlotWidget->SetItem(EquipmentItems[SlotIndex].ItemRowName, EquipmentItems[SlotIndex].Amount, EquipmentItems[SlotIndex].ChipStats);
		}
		else
		{
			SlotWidget->ClearItem();
		}
	}
}

void ULSInventoryWidget::SetEquipmentDragHighlight(const FName DraggedItemRowName)
{
	// 장착 대상 슬롯 타입(=슬롯 인덱스)을 결정한다. 장착 불가면 Count가 나와 어느 칸도 매칭되지 않는다.
	const ELSEquipmentSlot TargetType = LSInventorySlotUtils::ResolveEquipmentSlotType(DraggedItemRowName);

	// ELSEquipmentSlot 순서와 일치해야 한다(인덱스 = 슬롯 타입). RebuildEquipmentSlots와 동일 배열.
	ULSItemSlotWidget* SlotWidgets[] = { WeaponSlot, ProcessorSlot, CoreSlot, ActuatorSlot, FrameSlot };
	for (int32 SlotIndex = 0; SlotIndex < UE_ARRAY_COUNT(SlotWidgets); ++SlotIndex)
	{
		if (ULSItemSlotWidget* SlotWidget = SlotWidgets[SlotIndex])
		{
			SlotWidget->SetEquipCandidateHighlight(TargetType == static_cast<ELSEquipmentSlot>(SlotIndex));
		}
	}
}

void ULSInventoryWidget::ClearEquipmentDragHighlight()
{
	ULSItemSlotWidget* SlotWidgets[] = { WeaponSlot, ProcessorSlot, CoreSlot, ActuatorSlot, FrameSlot };
	for (ULSItemSlotWidget* SlotWidget : SlotWidgets)
	{
		if (SlotWidget)
		{
			SlotWidget->SetEquipCandidateHighlight(false);
		}
	}
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
		PlayerController->RefreshOpenChipStationWidget();
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
