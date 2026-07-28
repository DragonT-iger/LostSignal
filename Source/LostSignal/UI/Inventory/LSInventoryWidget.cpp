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
#include "UI/Common/LSConfirmDialogWidget.h"
#include "UI/Inventory/LSInventoryDragDropOperation.h"
#include "UI/Inventory/LSItemSlotWidget.h"
#include "UI/Inventory/LSSlotWidgetSync.h"
#include "UI/LootDrop/LSLootDropWidget.h"
#include "UI/LSUILayer.h"

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

	if (!QuickSlotBar)
	{
		UE_LOG(LogLS, Warning, TEXT("QuickSlotBar is not bound on %s."), *GetNameSafe(this));
	}
	else
	{
		// 기본은 표시. 폰 경로(ShowInventoryWidgetInternal)를 타는 레이드는 매 오픈 시 값을 덮어써 루팅 박스에서만 숨긴다.
		// 폰 없이 열리는 로비 인벤토리는 이 경로를 안 타므로 기본 표시로 남아 Tab/메뉴로 열어도 바가 보인다.
		SetQuickSlotBarVisible(true);
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
			SlotWidget.SetSlotLayoutSize(InventoryItemSlotSize);
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
					// 인벤토리/Safe/창고/칩스테이션 등 열려 있는 패널 전체를 데이터에서 다시 그린다(부분/누락 갱신 금지).
					PlayerController->RefreshAllInventoryUI();
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

	if (bChanged)
	{
		// authority면 즉시, 비-authority면 로컬 미러를 다시 그린다(서버 미러 RPC가 오면 funnel이 멱등 재적용).
		PlayerController->RefreshAllInventoryUI();
	}

	return bChanged;
}

bool ULSInventoryWidget::HandleEquipmentSlotDrop(const ELSInventorySlotArea FromSlotArea, const int32 FromSlotIndex, const ELSInventorySlotArea ToSlotArea, const int32 ToSlotIndex)
{
	if (FromSlotIndex == INDEX_NONE || ToSlotIndex == INDEX_NONE)
	{
		return false;
	}

	ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer());
	if (!PlayerController)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot handle equipment slot drop because owning player controller is invalid on %s."), *GetNameSafe(this));
		return false;
	}

	// 잠긴 보호 슬롯(적재 프로토콜 감소분 등)은 장비 이동 원본/대상으로 쓸 수 없다.
	if (IsSlotLocked(FromSlotArea, FromSlotIndex) || IsSlotLocked(ToSlotArea, ToSlotIndex))
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot handle equipment slot drop because a slot is locked. FromArea=%d From=%d ToArea=%d To=%d"),
			static_cast<int32>(FromSlotArea), FromSlotIndex, static_cast<int32>(ToSlotArea), ToSlotIndex);
		return false;
	}

	// 레이드 중에는 장비도 세션 정식 영역이다. 인벤토리와 동일하게 서버 판정(DropInventorySlot)으로 라우팅한다.
	// (타입 불일치/용량 부족 등은 서버가 거부하고, 성공 시 미러 RPC가 UI를 funnel로 다시 그린다.)
	ULSRaidInventoryComponent* RaidInventory = PlayerController->GetRaidInventoryComponent();
	if (RaidInventory && RaidInventory->IsRaidActive())
	{
		const bool bChanged = PlayerController->DropInventorySlot(FromSlotArea, FromSlotIndex, ToSlotArea, ToSlotIndex);
		if (bChanged)
		{
			PlayerController->RefreshAllInventoryUI();
		}
		return bChanged;
	}

	// 로비: 클라 세이브(SaveSubsystem) 기반 장비 이동.
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
		// 장비 이동은 인벤토리 인덱스·창고·칩 리스트에 파급되므로, 열려 있는 패널 전체를 funnel로 다시 그린다.
		PlayerController->RefreshAllInventoryUI();
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

	// 룻박스(소스) 슬롯은 TransferLootSlotToInventorySlot이 내부에서 다시 그린다. 대상(인벤토리 계열)은 funnel로 전체를 다시 그린다.
	const bool bTransferred = LootDropWidget->TransferLootSlotToInventorySlot(LootSlotIndex, ToSlotArea, ToSlotIndex);
	if (bTransferred)
	{
		if (ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer()))
		{
			PlayerController->RefreshAllInventoryUI();
		}
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

	// 장착된 장비를 창 밖으로 드래그: 레이드 중에는 월드 드랍을 허용한다(익스트렉션 리스크 / 장착칸 직행 허용).
	// 로비에서는 드랍하지 않는다(드래그 취소 시 장착 유지 — 로비 장비 월드 드랍은 범위 밖).
	if (DragOperation.SourceSlotArea == ELSInventorySlotArea::Equipment)
	{
		const ALSPlayerControllerBase* LSPlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer());
		const ULSRaidInventoryComponent* RaidInventory = LSPlayerController ? LSPlayerController->GetRaidInventoryComponent() : nullptr;
		if (!RaidInventory || !RaidInventory->IsRaidActive())
		{
			return false;
		}
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
			SlotWidget.SetSlotLayoutSize(InventoryItemSlotSize);
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

void ULSInventoryWidget::SetQuickSlotBarVisible(const bool bVisible)
{
	if (!QuickSlotBar)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot update QuickSlotBar visibility because QuickSlotBar is not bound on %s."), *GetNameSafe(this));
		return;
	}

	// 숨길 때는 Collapsed로 자리도 차지하지 않게 한다. 보일 때는 자식 슬롯이 드롭/클릭을 받도록 SelfHitTestInvisible.
	QuickSlotBar->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
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
	const bool bChanged = SaveSubsystem->TransferAllInventoryToWarehouse(bStoppedBecauseFull);

	if (bChanged)
	{
		PlayerController->RefreshAllInventoryUI();
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
				if (PlayerController->SortRaidInventory())
				{
					PlayerController->RefreshAllInventoryUI();
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

	// 정렬은 인벤토리를 압축해 인덱스를 재배치하므로, 열려 있는 인벤토리 계열 패널 전체를 funnel로 다시 그린다.
	if (ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer()))
	{
		PlayerController->RefreshAllInventoryUI();
	}
}

void ULSInventoryWidget::RebuildEquipmentSlots()
{
	// 장비는 레이드 세션의 정식 영역이다. 레이드 중에는 서버가 미러링한 세션 장비를,
	// 로비에서는 클라 세이브 장비를 표시한다. 두 경우 모두 잠그지 않고 드래그/변경을 허용한다.
	static const TArray<FLSSessionItem> EmptyEquipment;
	const TArray<FLSSessionItem>* EquipmentSource = nullptr;

	if (ALSPlayerControllerBase* LSPlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer()))
	{
		if (ULSRaidInventoryComponent* RaidInventory = LSPlayerController->GetRaidInventoryComponent())
		{
			if (RaidInventory->IsRaidActive())
			{
				EquipmentSource = &RaidInventory->GetSessionEquipmentSlots();
			}
		}
	}

	if (!EquipmentSource)
	{
		UGameInstance* GameInstance = GetGameInstance();
		ULSSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
		EquipmentSource = SaveSubsystem ? &SaveSubsystem->GetEquipmentSlots() : &EmptyEquipment;
	}

	const TArray<FLSSessionItem>& EquipmentItems = *EquipmentSource;

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
		SlotWidget->SetSlotLayoutSize(InventoryItemSlotSize);
		SlotWidget->SetSlotContext(this, ELSInventorySlotArea::Equipment, SlotIndex, bHasSlotItem);
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

void ULSInventoryWidget::ShowInventoryFullNotification()
{
	// 이미 알림이 떠 있으면 중복 생성하지 않는다(Shift 쓸기로 재호출돼도 무해).
	if (ActiveNotificationDialog && ActiveNotificationDialog->IsInViewport())
	{
		return;
	}

	if (!ConfirmDialogClass)
	{
		UE_LOG(LogLS, Warning, TEXT("[Inventory] ConfirmDialogClass is not set on %s. Check WBP_Inventory."), *GetNameSafe(this));
		return;
	}

	// Shift 빠른이동 제스처의 마우스 Down은 이미 슬롯이 소비했다. 지금 다이얼로그를 띄우면 확인 버튼이
	// 짝 Down을 못 받아 첫 클릭이 씹히므로, 제스처가 끝난 다음 틱에 생성한다(칩 스테이션과 동일 패턴).
	UWorld* World = GetWorld();
	if (!World)
	{
		PresentInventoryFullNotification();
		return;
	}

	TWeakObjectPtr<ULSInventoryWidget> WeakThis(this);
	World->GetTimerManager().SetTimerForNextTick([WeakThis]()
	{
		if (ULSInventoryWidget* StrongThis = WeakThis.Get())
		{
			StrongThis->PresentInventoryFullNotification();
		}
	});
}

void ULSInventoryWidget::PresentInventoryFullNotification()
{
	// 다음 틱 사이에 이미 알림이 떠 있거나 클래스가 사라졌을 수 있어 다시 확인한다.
	if (ActiveNotificationDialog && ActiveNotificationDialog->IsInViewport())
	{
		return;
	}

	if (!ConfirmDialogClass)
	{
		UE_LOG(LogLS, Warning, TEXT("[Inventory] ConfirmDialogClass is not set on %s. Check WBP_Inventory."), *GetNameSafe(this));
		return;
	}

	APlayerController* OwningPlayer = GetOwningPlayer();
	ULSConfirmDialogWidget* Dialog = OwningPlayer
		? CreateWidget<ULSConfirmDialogWidget>(OwningPlayer, ConfirmDialogClass)
		: CreateWidget<ULSConfirmDialogWidget>(this, ConfirmDialogClass);
	if (!Dialog)
	{
		UE_LOG(LogLS, Warning, TEXT("[Inventory] Failed to create inventory-full notification dialog on %s."), *GetNameSafe(this));
		return;
	}

	Dialog->SetMessage(NSLOCTEXT("LSInventory", "InventoryFull", "인벤토리가 가득 찼습니다."));
	// 정보 알림이라 확인/취소/ESC 어느 쪽이든 그냥 닫힌다.
	Dialog->OnConfirmed.AddDynamic(this, &ULSInventoryWidget::HandleNotificationDialogClosed);
	Dialog->OnCancelled.AddDynamic(this, &ULSInventoryWidget::HandleNotificationDialogClosed);

	Dialog->AddToViewport(LSUILayer::ModalPanelDialog);
	ActiveNotificationDialog = Dialog;
}

void ULSInventoryWidget::HandleNotificationDialogClosed()
{
	// 다이얼로그는 스스로 뷰포트에서 제거되므로 참조만 비운다.
	ActiveNotificationDialog = nullptr;
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

bool ULSInventoryWidget::CanAcceptEquipmentDrop(const FName ItemRowName, const int32 EquipmentSlotIndex) const
{
	if (EquipmentSlotIndex < 0 || EquipmentSlotIndex >= static_cast<int32>(ELSEquipmentSlot::Count))
	{
		return false;
	}

	return LSInventorySlotUtils::ResolveEquipmentSlotType(ItemRowName) == static_cast<ELSEquipmentSlot>(EquipmentSlotIndex);
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
		// 월드에 버려 슬롯이 비고 인덱스가 재배치되므로, 열려 있는 인벤토리 계열 패널 전체를 funnel로 다시 그린다.
		PlayerController->RefreshAllInventoryUI();
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
