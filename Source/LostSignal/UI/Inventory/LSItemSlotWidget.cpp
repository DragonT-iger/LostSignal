#include "UI/Inventory/LSItemSlotWidget.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Characters/LSPlayerCharacter.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Core/LSPlayerControllerBase.h"
#include "Engine/Texture2D.h"
#include "InputCoreTypes.h"
#include "Inventory/LSInventorySlotUtils.h"
#include "LostSignal.h"
#include "Session/LSSaveSubsystem.h"
#include "UI/ChipSystem/LSChipEquipmentSlotWidget.h"
#include "UI/ChipSystem/LSChipStationWidget.h"
#include "UI/Inventory/LSInventoryDragDropOperation.h"
#include "UI/Inventory/LSInventoryWidget.h"
#include "UI/LootDrop/LSLootDropWidget.h"
#include "UI/Storage/LSLobbyStorageWidget.h"

namespace
{
// 빈 슬롯(기본 텍스처)이 적용된 상태를 나타내는 예약 키. 실제 아이템 행 이름과 겹치지 않는다.
const FName EmptySlotIconKey(TEXT("__LSEmptySlot__"));
}

void ULSItemSlotWidget::SetItem(const FName ItemRowName, const int32 Amount, const TArray<FLSChipResolvedStat>& ChipStats)
{
	if (!ItemIconImage)
	{
		UE_LOG(LogLS, Warning, TEXT("ItemIconImage is not bound on %s."), *GetNameSafe(this));
		return;
	}

	if (!AmountText)
	{
		UE_LOG(LogLS, Warning, TEXT("AmountText is not bound on %s."), *GetNameSafe(this));
		return;
	}

	if (ItemRowName.IsNone())
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot set item slot because ItemRowName is none on %s."), *GetNameSafe(this));
		ClearItem();
		return;
	}

	// 같은 아이템을 다시 표시하는 경우 동기 텍스처 로딩과 브러시 갱신을 건너뛴다.
	if (DisplayedIconKey != ItemRowName)
	{
		UTexture2D* IconTexture = LoadIconTextureByRowName(ItemRowName);
		const bool bUsedItemIcon = IconTexture != nullptr;
		if (!IconTexture)
		{
			IconTexture = LoadDefaultIconTexture();
			UE_LOG(LogLS, Warning, TEXT("Using default item slot icon for row '%s' on %s."), *ItemRowName.ToString(), *GetNameSafe(this));
		}

		if (IconTexture)
		{
			ItemIconImage->SetBrushFromTexture(IconTexture);
		}

		// 실제 아이템 아이콘이 적용된 경우에만 캐시한다. 기본 아이콘 대체 시에는 다음 호출에서 다시 시도한다.
		DisplayedIconKey = bUsedItemIcon ? ItemRowName : NAME_None;
	}

	ApplyHoverVisual();
	ItemIconImage->SetVisibility(ESlateVisibility::Visible);
	AmountText->SetText(FText::AsNumber(Amount));
	AmountText->SetVisibility(Amount > 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	SetTooltipItem(ItemRowName, Amount, ChipStats);
	DragItemRowName = ItemRowName;
	DragAmount = Amount;
	DragChipStats = ChipStats;
	bHasItem = true;
}

void ULSItemSlotWidget::ClearItem()
{
	if (!ItemIconImage)
	{
		UE_LOG(LogLS, Warning, TEXT("ItemIconImage is not bound on %s."), *GetNameSafe(this));
		return;
	}

	if (!AmountText)
	{
		UE_LOG(LogLS, Warning, TEXT("AmountText is not bound on %s."), *GetNameSafe(this));
		return;
	}

	// 이미 빈 슬롯 텍스처가 적용돼 있으면 다시 로드하지 않는다.
	if (DisplayedIconKey != EmptySlotIconKey)
	{
		if (UTexture2D* DefaultIconTexture = LoadSlotDefaultTexture())
		{
			ItemIconImage->SetBrushFromTexture(DefaultIconTexture);
			DisplayedIconKey = EmptySlotIconKey;
		}
		else
		{
			UE_LOG(LogLS, Warning, TEXT("ClearItem could not load default icon on %s."), *GetNameSafe(this));
			DisplayedIconKey = NAME_None;
		}
	}

	ApplyHoverVisual();
	ItemIconImage->SetVisibility(ESlateVisibility::Visible);
	AmountText->SetText(FText::GetEmpty());
	AmountText->SetVisibility(ESlateVisibility::Collapsed);
	bHasItem = false;
	DragItemRowName = NAME_None;
	DragAmount = 0;
	DragChipStats.Reset();
	ClearTooltipItem();
}

void ULSItemSlotWidget::SetSlotLocked(const bool bInLocked)
{
	bIsLocked = bInLocked;
	ApplyHoverVisual();
}

void ULSItemSlotWidget::SetDisplayOnlySlotContext()
{
	InventoryWidget.Reset();
	LootDropWidget.Reset();
	LobbyStorageWidget.Reset();
	ChipStationWidget.Reset();
	ChipEquipmentSlotWidget.Reset();
	SlotArea = ELSInventorySlotArea::Inventory;
	SlotIndex = INDEX_NONE;
	EquipmentSlotIndex = INDEX_NONE;
	bHasItem = false;
	bIsLocked = false;
}

void ULSItemSlotWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (!bHasItem)
	{
		ClearItem();
	}
}

void ULSItemSlotWidget::SetSlotContext(ULSInventoryWidget* InInventoryWidget, const ELSInventorySlotArea InSlotArea, const int32 InSlotIndex, const bool bInHasItem, const bool bInLocked)
{
	InventoryWidget = InInventoryWidget;
	LootDropWidget.Reset();
	LobbyStorageWidget.Reset();
	ChipStationWidget.Reset();
	ChipEquipmentSlotWidget.Reset();
	SlotArea = InSlotArea;
	SlotIndex = InSlotIndex;
	EquipmentSlotIndex = INDEX_NONE;
	bHasItem = bInHasItem;
	bIsLocked = bInLocked;
}

void ULSItemSlotWidget::SetLootSlotContext(ULSLootDropWidget* InLootDropWidget, const int32 InSlotIndex, const bool bInHasItem)
{
	LootDropWidget = InLootDropWidget;
	InventoryWidget.Reset();
	LobbyStorageWidget.Reset();
	ChipStationWidget.Reset();
	ChipEquipmentSlotWidget.Reset();
	SlotArea = ELSInventorySlotArea::Inventory;
	SlotIndex = InSlotIndex;
	EquipmentSlotIndex = INDEX_NONE;
	bHasItem = bInHasItem;
	bIsLocked = false;
}

void ULSItemSlotWidget::SetWarehouseSlotContext(ULSLobbyStorageWidget* InStorageWidget, const ELSInventorySlotArea InSlotArea, const int32 InSlotIndex, const bool bInHasItem)
{
	LobbyStorageWidget = InStorageWidget;
	InventoryWidget.Reset();
	LootDropWidget.Reset();
	ChipStationWidget.Reset();
	ChipEquipmentSlotWidget.Reset();
	SlotArea = InSlotArea;
	SlotIndex = InSlotIndex;
	EquipmentSlotIndex = INDEX_NONE;
	bHasItem = bInHasItem;
	bIsLocked = false;
}

void ULSItemSlotWidget::SetChipStationSlotContext(ULSChipStationWidget* InChipStationWidget, const ELSInventorySlotArea InSourceArea, const int32 InSourceSlotIndex, const FName InItemRowName, const int32 InAmount, const TArray<FLSChipResolvedStat>& InChipStats)
{
	ChipStationWidget = InChipStationWidget;
	InventoryWidget.Reset();
	LootDropWidget.Reset();
	LobbyStorageWidget.Reset();
	ChipEquipmentSlotWidget.Reset();
	SlotArea = InSourceArea;
	SlotIndex = InSourceSlotIndex;
	EquipmentSlotIndex = INDEX_NONE;
	DragItemRowName = InItemRowName;
	DragAmount = InAmount;
	DragChipStats = InChipStats;
	bHasItem = !InItemRowName.IsNone() && InAmount > 0;
	bIsLocked = false;
}

void ULSItemSlotWidget::SetChipEquipmentSlotContext(ULSChipEquipmentSlotWidget* InChipEquipmentSlotWidget, ULSChipStationWidget* InChipStationWidget, const int32 InEquipmentSlotIndex)
{
	ChipEquipmentSlotWidget = InChipEquipmentSlotWidget;
	InventoryWidget.Reset();
	LootDropWidget.Reset();
	LobbyStorageWidget.Reset();
	ChipStationWidget = InChipStationWidget;
	SlotArea = ELSInventorySlotArea::Inventory;
	SlotIndex = INDEX_NONE;
	EquipmentSlotIndex = InEquipmentSlotIndex;
	bIsLocked = false;
}

void ULSItemSlotWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	bIsHovered = true;
	if (ULSLootDropWidget* OwningLootDropWidget = LootDropWidget.Get())
	{
		OwningLootDropWidget->NotifyLootSlotHovered(SlotIndex);
	}
	if (IsQuickTransferPointerEvent(InMouseEvent))
	{
		TryHandleQuickTransfer();
	}
	ApplyHoverVisual();
}

void ULSItemSlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	bIsHovered = false;
	if (ULSLootDropWidget* OwningLootDropWidget = LootDropWidget.Get())
	{
		OwningLootDropWidget->NotifyLootSlotUnhovered(SlotIndex);
	}
	ApplyHoverVisual();
}

FReply ULSItemSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && InMouseEvent.IsShiftDown())
	{
		TryHandleQuickTransfer();
		return FReply::Handled();
	}

	if (!CanStartItemDrag())
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
}

FReply ULSItemSlotWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (IsQuickTransferPointerEvent(InMouseEvent))
	{
		TryHandleQuickTransfer();
	}

	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

void ULSItemSlotWidget::NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragEnter(InGeometry, InDragDropEvent, InOperation);

	bIsDragTarget = IsValidInventoryDropTarget(InOperation) || IsValidLootDropTarget(InOperation) || IsValidWarehouseDropTarget(InOperation);
	ApplyHoverVisual();
}

void ULSItemSlotWidget::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragLeave(InDragDropEvent, InOperation);

	bIsDragTarget = false;
	ApplyHoverVisual();
}

void ULSItemSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	if (!CanStartItemDrag())
	{
		return;
	}

	ULSInventoryDragDropOperation* DragOperation = Cast<ULSInventoryDragDropOperation>(
		UWidgetBlueprintLibrary::CreateDragDropOperation(ULSInventoryDragDropOperation::StaticClass()));
	if (!DragOperation)
	{
		UE_LOG(LogLS, Warning, TEXT("Failed to create inventory drag operation on %s."), *GetNameSafe(this));
		return;
	}

	DragOperation->SourceInventoryWidget = InventoryWidget.Get();
	DragOperation->SourceLootDropWidget = LootDropWidget.Get();
	DragOperation->SourceLobbyStorageWidget = LobbyStorageWidget.Get();
	DragOperation->SourceChipStationWidget = ChipStationWidget.Get();
	DragOperation->SourceChipEquipmentSlotWidget = ChipEquipmentSlotWidget.Get();
	DragOperation->SourceSlotWidget = this;
	DragOperation->SourceSlotIndex = SlotIndex;
	DragOperation->SourceEquipmentSlotIndex = EquipmentSlotIndex;
	DragOperation->SourceSlotArea = SlotArea;
	DragOperation->DragItemRowName = DragItemRowName;
	DragOperation->DragAmount = DragAmount;
	DragOperation->DragChipStats = DragChipStats;
	// 드래그 비주얼은 반드시 별도 인스턴스로 만든다. 살아있는 소스 위젯(this)을 비주얼로 쓰면
	// 드래그 종료 후 그 위젯의 Slate 드래그 감지가 영구히 죽어, 풀링 재사용 시 해당 슬롯이 드래그 불가가 된다.
	// SetItem만 호출해 bHasItem=true로 둔다(SetDisplayOnlySlotContext는 bHasItem=false로 만들어 PreConstruct에서 아이콘이 지워진다).
	if (APlayerController* OwningPlayer = GetOwningPlayer())
	{
		if (ULSItemSlotWidget* DragVisual = CreateWidget<ULSItemSlotWidget>(OwningPlayer, GetClass()))
		{
			DragVisual->SetItem(DragItemRowName, DragAmount, DragChipStats);
			DragOperation->DefaultDragVisual = DragVisual;
		}
	}
	DragOperation->Pivot = EDragPivot::MouseDown;
	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetRenderOpacity(0.25f);
	OutOperation = DragOperation;
}

bool ULSItemSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	ULSInventoryDragDropOperation* DragOperation = Cast<ULSInventoryDragDropOperation>(InOperation);
	if (!DragOperation)
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	bIsDragTarget = false;
	ApplyHoverVisual();

	if (bIsLocked)
	{
		return false;
	}

	if (ULSChipEquipmentSlotWidget* EquipmentSlotWidget = ChipEquipmentSlotWidget.Get())
	{
		return EquipmentSlotWidget->HandleChipDrop(*DragOperation);
	}

	if (ULSChipStationWidget* TargetChipStationWidget = ChipStationWidget.Get())
	{
		if (DragOperation->SourceChipEquipmentSlotWidget)
		{
			return TargetChipStationWidget->SwapEquippedChipWithStoredSlot(*DragOperation, SlotArea, SlotIndex);
		}

		return false;
	}

	if (ULSLootDropWidget* TargetLootDropWidget = LootDropWidget.Get())
	{
		if (DragOperation->SourceLootDropWidget)
		{
			if (DragOperation->SourceLootDropWidget != TargetLootDropWidget)
			{
				UE_LOG(LogLS, Warning, TEXT("Cannot drop loot slot because source/target loot drop widget does not match."));
				return false;
			}

			return TargetLootDropWidget->DropLootSlot(DragOperation->SourceSlotIndex, SlotIndex);
		}

		if (!DragOperation->SourceInventoryWidget)
		{
			return false;
		}

		const bool bTransferred = TargetLootDropWidget->TransferInventorySlotToLootSlot(
			DragOperation->SourceSlotArea,
			DragOperation->SourceSlotIndex,
			SlotIndex);
		if (bTransferred)
		{
			DragOperation->SourceInventoryWidget->RebuildInventorySlots();
			DragOperation->SourceInventoryWidget->RebuildConfirmedStorageSlots();
		}

		return bTransferred;
	}

	if (ULSLobbyStorageWidget* TargetStorageWidget = LobbyStorageWidget.Get())
	{
		if (!DragOperation->SourceInventoryWidget && !DragOperation->SourceLobbyStorageWidget)
		{
			return false;
		}

		const ELSInventorySlotArea SourceArea = DragOperation->SourceLobbyStorageWidget
			? ELSInventorySlotArea::Warehouse
			: DragOperation->SourceSlotArea;

		return TargetStorageWidget->HandleStorageSlotDrop(SourceArea, DragOperation->SourceSlotIndex, SlotIndex);
	}

	ULSInventoryWidget* TargetInventoryWidget = InventoryWidget.Get();
	if (TargetInventoryWidget && DragOperation->SourceLootDropWidget)
	{
		return TargetInventoryWidget->HandleLootSlotDrop(
			DragOperation->SourceLootDropWidget,
			DragOperation->SourceSlotIndex,
			SlotArea,
			SlotIndex);
	}

	if (TargetInventoryWidget && DragOperation->SourceLobbyStorageWidget)
	{
		const bool bDropped = TargetInventoryWidget->HandleInventorySlotDrop(
			ELSInventorySlotArea::Warehouse,
			DragOperation->SourceSlotIndex,
			SlotArea,
			SlotIndex);
		if (bDropped)
		{
			DragOperation->SourceLobbyStorageWidget->RefreshStorage();
		}
		return bDropped;
	}

	if (!TargetInventoryWidget || DragOperation->SourceInventoryWidget != TargetInventoryWidget)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot drop inventory slot because source/target inventory widget does not match."));
		return false;
	}

	return TargetInventoryWidget->HandleInventorySlotDrop(
		DragOperation->SourceSlotArea,
		DragOperation->SourceSlotIndex,
		SlotArea,
		SlotIndex);
}

void ULSItemSlotWidget::RestoreDragSourceVisual()
{
	SetVisibility(ESlateVisibility::Visible);
	SetRenderOpacity(1.0f);
	ApplyHoverVisual();
}

void ULSItemSlotWidget::ResetTransientSlotState()
{
	bIsHovered = false;
	bIsDragTarget = false;
	SetVisibility(ESlateVisibility::Visible);
	SetRenderOpacity(1.0f);
}

void ULSItemSlotWidget::ApplyHoverVisual()
{
	if (!ItemIconImage)
	{
		return;
	}

	if (bIsLocked)
	{
		ItemIconImage->SetColorAndOpacity(LockedIconTint);
		return;
	}

	if (bIsDragTarget)
	{
		ItemIconImage->SetColorAndOpacity(DragTargetIconTint);
		return;
	}

	ItemIconImage->SetColorAndOpacity(bIsHovered ? HoveredIconTint : NormalIconTint);
}

bool ULSItemSlotWidget::CanStartItemDrag() const
{
	if (!bHasItem)
	{
		return false;
	}

	if (bIsLocked)
	{
		return false;
	}

	if (ChipStationWidget.IsValid())
	{
		if (ChipEquipmentSlotWidget.IsValid())
		{
			return EquipmentSlotIndex != INDEX_NONE && !DragItemRowName.IsNone() && DragAmount > 0;
		}

		return SlotIndex != INDEX_NONE && !DragItemRowName.IsNone() && DragAmount > 0;
	}

	return SlotIndex != INDEX_NONE && (InventoryWidget.IsValid() || LootDropWidget.IsValid() || LobbyStorageWidget.IsValid());
}

bool ULSItemSlotWidget::IsQuickTransferPointerEvent(const FPointerEvent& InMouseEvent) const
{
	return InMouseEvent.IsShiftDown() && InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton);
}

bool ULSItemSlotWidget::TryHandleQuickTransfer()
{
	// 칩 장착 슬롯: Shift+좌클릭 -> 창고로 해제. (장착 슬롯은 SlotIndex가 INDEX_NONE이라 아래 가드보다 먼저 처리한다.)
	if (ChipEquipmentSlotWidget.IsValid())
	{
		return TryHandleChipEquipmentQuickTransfer();
	}

	if (!bHasItem || SlotIndex == INDEX_NONE)
	{
		return false;
	}

	// 칩 목록 슬롯(인벤토리/창고): Shift+좌클릭 -> 첫 빈 장착 슬롯에 순서대로 장착.
	if (ChipStationWidget.IsValid())
	{
		return TryHandleChipStationQuickTransfer();
	}

	if (LootDropWidget.IsValid())
	{
		return TryHandleLootQuickTransfer();
	}

	if (InventoryWidget.IsValid() && !bIsLocked)
	{
		return TryHandleInventoryQuickTransfer();
	}

	if (LobbyStorageWidget.IsValid())
	{
		return TryHandleWarehouseQuickTransfer();
	}

	return false;
}

bool ULSItemSlotWidget::TryHandleLootQuickTransfer()
{
	ULSLootDropWidget* OwningLootDropWidget = LootDropWidget.Get();
	if (!OwningLootDropWidget || !OwningLootDropWidget->TransferLootSlotToInventory(SlotIndex))
	{
		return false;
	}

	bHasItem = false;
	APlayerController* OwningPlayer = GetOwningPlayer();
	if (OwningPlayer && OwningPlayer->HasAuthority())
	{
		if (ALSPlayerCharacter* PlayerCharacter = Cast<ALSPlayerCharacter>(GetOwningPlayerPawn()))
		{
			PlayerCharacter->RebuildInventoryWidgetSlots();
		}
	}
	return true;
}

bool ULSItemSlotWidget::TryHandleInventoryQuickTransfer()
{
	ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer());
	if (!PlayerController || !PlayerController->TransferInventorySlotToOpenContainer(SlotArea, SlotIndex))
	{
		return false;
	}

	if (PlayerController->IsLobbyStorageWidgetOpen())
	{
		RefreshStoredSlotVisual();
	}
	else if (PlayerController->HasAuthority())
	{
		InventoryWidget->RebuildInventorySlots();
		InventoryWidget->RebuildConfirmedStorageSlots();
	}
	else
	{
		ClearItem();
	}
	return true;
}

bool ULSItemSlotWidget::TryHandleWarehouseQuickTransfer()
{
	ULSLobbyStorageWidget* StorageWidget = LobbyStorageWidget.Get();
	if (!StorageWidget || !StorageWidget->TransferStorageSlotToInventory(SlotIndex, false))
	{
		return false;
	}

	ClearItem();
	return true;
}

bool ULSItemSlotWidget::TryHandleChipEquipmentQuickTransfer()
{
	if (!bHasItem || EquipmentSlotIndex == INDEX_NONE)
	{
		return false;
	}

	ULSChipStationWidget* OwningChipStation = ChipStationWidget.Get();
	return OwningChipStation && OwningChipStation->QuickUnequipEquippedChipToWarehouse(EquipmentSlotIndex);
}

bool ULSItemSlotWidget::TryHandleChipStationQuickTransfer()
{
	ULSChipStationWidget* OwningChipStation = ChipStationWidget.Get();
	return OwningChipStation && OwningChipStation->QuickEquipChipToFirstEmptyHardwareSlot(SlotArea, SlotIndex);
}

void ULSItemSlotWidget::RefreshStoredSlotVisual()
{
	const UGameInstance* GameInstance = GetGameInstance();
	const ULSSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
	if (!SaveSubsystem)
	{
		ClearItem();
		return;
	}

	const TArray<FLSSessionItem>* Slots = nullptr;
	if (SlotArea == ELSInventorySlotArea::Inventory)
	{
		Slots = &SaveSubsystem->GetInventory();
	}
	else if (SlotArea == ELSInventorySlotArea::Safe)
	{
		Slots = &SaveSubsystem->GetSafeStash();
	}

	const FLSSessionItem* SlotItem = Slots && Slots->IsValidIndex(SlotIndex) ? &(*Slots)[SlotIndex] : nullptr;
	if (SlotItem && !SlotItem->ItemRowName.IsNone() && SlotItem->Amount > 0)
	{
		SetItem(SlotItem->ItemRowName, SlotItem->Amount, SlotItem->ChipStats);
		return;
	}

	ClearItem();
}

bool ULSItemSlotWidget::IsValidInventoryDropTarget(const UDragDropOperation* InOperation) const
{
	const ULSInventoryDragDropOperation* DragOperation = Cast<ULSInventoryDragDropOperation>(InOperation);
	if (!DragOperation || bIsLocked || !InventoryWidget.IsValid() || SlotIndex == INDEX_NONE || DragOperation->SourceSlotIndex == INDEX_NONE)
	{
		return false;
	}

	if (DragOperation->SourceLootDropWidget)
	{
		return true;
	}

	if (DragOperation->SourceInventoryWidget != InventoryWidget.Get())
	{
		return false;
	}

	return DragOperation->SourceSlotArea != SlotArea || DragOperation->SourceSlotIndex != SlotIndex;
}

bool ULSItemSlotWidget::IsValidLootDropTarget(const UDragDropOperation* InOperation) const
{
	const ULSInventoryDragDropOperation* DragOperation = Cast<ULSInventoryDragDropOperation>(InOperation);
	if (!DragOperation || !LootDropWidget.IsValid() || SlotIndex == INDEX_NONE || DragOperation->SourceSlotIndex == INDEX_NONE)
	{
		return false;
	}

	if (DragOperation->SourceInventoryWidget)
	{
		return true;
	}

	return DragOperation->SourceLootDropWidget == LootDropWidget.Get() && DragOperation->SourceSlotIndex != SlotIndex;
}

bool ULSItemSlotWidget::IsValidWarehouseDropTarget(const UDragDropOperation* InOperation) const
{
	const ULSInventoryDragDropOperation* DragOperation = Cast<ULSInventoryDragDropOperation>(InOperation);
	if (!DragOperation || !LobbyStorageWidget.IsValid() || SlotIndex == INDEX_NONE || DragOperation->SourceSlotIndex == INDEX_NONE)
	{
		return false;
	}

	if (DragOperation->SourceInventoryWidget)
	{
		return true;
	}

	if (DragOperation->SourceLobbyStorageWidget == LobbyStorageWidget.Get())
	{
		return DragOperation->SourceSlotIndex != SlotIndex;
	}

	return false;
}

UTexture2D* ULSItemSlotWidget::LoadIconTextureByRowName(const FName ItemRowName) const
{
	const FString IconObjectPath = BuildIconObjectPath(LSInventorySlotUtils::ResolveIconAssetNameFromRowName(ItemRowName), GetIconBaseFolderByRowName(ItemRowName));
	UTexture2D* IconTexture = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, *IconObjectPath));
	if (!IconTexture)
	{
		UE_LOG(LogLS, Warning, TEXT("Failed to load item icon '%s' for row '%s'."), *IconObjectPath, *ItemRowName.ToString());
	}

	return IconTexture;
}

UTexture2D* ULSItemSlotWidget::LoadSlotDefaultTexture() const
{
	if (DefaultSlotTexture)
	{
		return DefaultSlotTexture;
	}

	return LoadDefaultIconTexture();
}

UTexture2D* ULSItemSlotWidget::LoadDefaultIconTexture() const
{
	static const TCHAR* DefaultIconObjectPath = TEXT("/Engine/EngineResources/DefaultTexture.DefaultTexture");
	UTexture2D* DefaultIconTexture = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, DefaultIconObjectPath));
	if (!DefaultIconTexture)
	{
		UE_LOG(LogLS, Warning, TEXT("Failed to load default inventory icon '%s'."), DefaultIconObjectPath);
	}

	return DefaultIconTexture;
}

FString ULSItemSlotWidget::BuildIconObjectPath(const FString& IconNameOrPath, const FString& BaseFolder)
{
	if (IconNameOrPath.StartsWith(TEXT("/Game/")))
	{
		if (IconNameOrPath.Contains(TEXT(".")))
		{
			return IconNameOrPath;
		}

		FString AssetName;
		IconNameOrPath.Split(TEXT("/"), nullptr, &AssetName, ESearchCase::CaseSensitive, ESearchDir::FromEnd);
		return FString::Printf(TEXT("%s.%s"), *IconNameOrPath, *AssetName);
	}

	return FString::Printf(TEXT("%s%s.%s"), *BaseFolder, *IconNameOrPath, *IconNameOrPath);
}

FString ULSItemSlotWidget::GetIconBaseFolderByRowName(const FName ItemRowName)
{
	const FString RowNameString = ItemRowName.ToString();
	if (RowNameString.StartsWith(TEXT("Chip_")))
	{
		return TEXT("/Game/LostSignal/UI/Icons/Chips/");
	}

	if (RowNameString.StartsWith(TEXT("Weapon_")))
	{
		return TEXT("/Game/LostSignal/UI/Icons/Weapons/");
	}

	if (RowNameString.StartsWith(TEXT("Armor_")))
	{
		return TEXT("/Game/LostSignal/UI/Icons/Armors/");
	}

	return TEXT("/Game/LostSignal/UI/Icons/Items/");
}
