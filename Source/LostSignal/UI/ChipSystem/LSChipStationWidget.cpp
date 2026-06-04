#include "UI/ChipSystem/LSChipStationWidget.h"

#include "Components/Border.h"
#include "Components/WrapBox.h"
#include "Data/LSChipRow.h"
#include "Data/LSChipStats.h"
#include "Data/LSDropSettings.h"
#include "Engine/DataTable.h"
#include "Inventory/LSInventorySlotUtils.h"
#include "LostSignal.h"
#include "Session/LSSaveSubsystem.h"
#include "UI/ChipSystem/LSChipEquipmentSlotWidget.h"
#include "UI/ChipSystem/LSChipStatWidget.h"
#include "UI/Inventory/LSInventoryDragDropOperation.h"
#include "UI/Inventory/LSItemSlotWidget.h"
#include "UI/Protocol/LSProtocolWidget.h"

namespace
{
struct FLSChipStationViewItem
{
	ELSInventorySlotArea SourceArea = ELSInventorySlotArea::Inventory;
	int32 SourceSlotIndex = INDEX_NONE;
	FLSSessionItem Item;
	int32 Price = 0;
};

bool IsInventoryChipItem(const FLSSessionItem& Item)
{
	return LSInventorySlotUtils::IsFilled(Item) && Item.ItemRowName.ToString().StartsWith(TEXT("Chip_"));
}

UDataTable* LoadChipTable(const UObject* LogContext)
{
	const ULSDropSettings* Settings = GetDefault<ULSDropSettings>();
	UDataTable* ChipTable = Settings ? Settings->ChipTable.LoadSynchronous() : nullptr;
	if (!ChipTable)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot sort chip station slots by price because ChipTable is not set on %s."), *GetNameSafe(LogContext));
	}

	return ChipTable;
}

int32 ResolveChipPrice(UDataTable* ChipTable, const FName ItemRowName, const UObject* LogContext)
{
	if (!ChipTable)
	{
		return 0;
	}

	const FLSChipRow* Row = ChipTable->FindRow<FLSChipRow>(ItemRowName, TEXT("ChipStationSort"));
	if (!Row)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot resolve chip price for '%s' on %s."), *ItemRowName.ToString(), *GetNameSafe(LogContext));
		return 0;
	}

	return Row->Item_Cost;
}

int32 GetChipSourceSortOrder(const ELSInventorySlotArea SourceArea)
{
	return SourceArea == ELSInventorySlotArea::Inventory ? 0 : 1;
}

void AddChipViewItems(
	const TArray<FLSSessionItem>& Items,
	const ELSInventorySlotArea SourceArea,
	UDataTable* ChipTable,
	const UObject* LogContext,
	TArray<FLSChipStationViewItem>& OutViewItems)
{
	for (int32 SlotIndex = 0; SlotIndex < Items.Num(); ++SlotIndex)
	{
		if (!IsInventoryChipItem(Items[SlotIndex]))
		{
			continue;
		}

		FLSChipStationViewItem ViewItem;
		ViewItem.SourceArea = SourceArea;
		ViewItem.SourceSlotIndex = SlotIndex;
		ViewItem.Item = Items[SlotIndex];
		ViewItem.Price = ResolveChipPrice(ChipTable, ViewItem.Item.ItemRowName, LogContext);
		OutViewItems.Add(ViewItem);
	}
}

void SortChipViewItems(TArray<FLSChipStationViewItem>& ViewItems)
{
	ViewItems.Sort([](const FLSChipStationViewItem& Left, const FLSChipStationViewItem& Right)
	{
		if (Left.Price != Right.Price)
		{
			return Left.Price > Right.Price;
		}

		const int32 LeftSourceOrder = GetChipSourceSortOrder(Left.SourceArea);
		const int32 RightSourceOrder = GetChipSourceSortOrder(Right.SourceArea);
		return LeftSourceOrder != RightSourceOrder
			? LeftSourceOrder < RightSourceOrder
			: Left.SourceSlotIndex < Right.SourceSlotIndex;
	});
}
}

void ULSChipStationWidget::NativeConstruct()
{
	Super::NativeConstruct();

	InitializeEquipmentSlots();
	UE_LOG(LogLS, Warning, TEXT("[ChipStation] NativeConstruct -> RefreshChipStation (this=%s)"), *GetNameSafe(this));
	RefreshChipStation();
}

void ULSChipStationWidget::NativeDestruct()
{
	Super::NativeDestruct();
}

bool ULSChipStationWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	ULSInventoryDragDropOperation* DragOperation = Cast<ULSInventoryDragDropOperation>(InOperation);
	if (DragOperation && DragOperation->SourceChipEquipmentSlotWidget && IsPointerInsideChipSlotBorder(InDragDropEvent.GetScreenSpacePosition()))
	{
		return UnequipChipToWarehouse(*DragOperation);
	}

	return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
}

void ULSChipStationWidget::RefreshChipStation_Implementation()
{
	UE_LOG(LogLS, Warning, TEXT("[ChipStation] RefreshChipStation_Implementation 진입"));

	// [임시 테스트] 8개 칸을 임의값(StatValue, SignalLoss)으로 채워 화면 표시 확인.
	// 실제 칩 장착/메모리/신호 게이지 연동 전까지의 더미 데이터다.
	RefreshChipSlots();
	RefreshEquipmentSlots();
	SetChipStat(TEXT("Chip_Attack"),              10, 5);
	SetChipStat(TEXT("Chip_Critical_Rate"),        7, 3);
	SetChipStat(TEXT("Chip_Critical_Damage"),     12, 6);
	SetChipStat(TEXT("Chip_Defense_Penetration"),  8, 4);
	SetChipStat(TEXT("Chip_Health"),              30, 8);
	SetChipStat(TEXT("Chip_Defense"),             15, 5);
	SetChipStat(TEXT("Chip_Attack_Speed"),         6, 2);
	SetChipStat(TEXT("Chip_Move_Speed"),           5, 2);

	SetProtocolWidget(Protocol_Survival, TEXT("Protocol_Survival"), 3, 3);
	SetProtocolWidget(Protocol_Carrying, TEXT("Protocol_Carrying"), 2, 1);
	SetProtocolWidget(Protocol_Battle, TEXT("Protocol_Battle"), 5, 6);
	SetProtocolWidget(Protocol_Navigation, TEXT("Protocol_Navigation"), 1, 0);
}

void ULSChipStationWidget::RefreshChipSlots()
{
	if (!ChipSlotWrapBox)
	{
		UE_LOG(LogLS, Warning, TEXT("ChipSlotWrapBox is not bound on %s."), *GetNameSafe(this));
		return;
	}

	ChipSlotWrapBox->ClearChildren();

	if (!ItemSlotWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("ItemSlotWidgetClass is not set on %s."), *GetNameSafe(this));
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	ULSSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
	if (!SaveSubsystem)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot refresh chip slots because SaveSubsystem is missing on %s."), *GetNameSafe(this));
		return;
	}

	UDataTable* ChipTable = LoadChipTable(this);
	TArray<FLSChipStationViewItem> ViewItems;
	AddChipViewItems(SaveSubsystem->GetInventory(), ELSInventorySlotArea::Inventory, ChipTable, this, ViewItems);
	AddChipViewItems(SaveSubsystem->GetWarehouseItems(), ELSInventorySlotArea::Warehouse, ChipTable, this, ViewItems);
	SortChipViewItems(ViewItems);

	for (const FLSChipStationViewItem& ViewItem : ViewItems)
	{
		ULSItemSlotWidget* SlotWidget = CreateChipSlotWidget();
		if (!SlotWidget)
		{
			continue;
		}

		SlotWidget->SetItem(ViewItem.Item.ItemRowName, ViewItem.Item.Amount, ViewItem.Item.ChipStats);
		SlotWidget->SetChipStationSlotContext(this, ViewItem.SourceArea, ViewItem.SourceSlotIndex, ViewItem.Item.ItemRowName, ViewItem.Item.Amount, ViewItem.Item.ChipStats);
		ChipSlotWrapBox->AddChildToWrapBox(SlotWidget);
	}
}

void ULSChipStationWidget::RefreshEquipmentSlots()
{
	UGameInstance* GameInstance = GetGameInstance();
	ULSSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
	if (!SaveSubsystem)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot refresh chip equipment slots because SaveSubsystem is missing on %s."), *GetNameSafe(this));
		return;
	}

	const TArray<FLSSessionItem>& EquipmentItems = SaveSubsystem->GetChipEquipmentSlots();
	TArray<ULSChipEquipmentSlotWidget*> EquipmentSlots = {
		EquipmentSlot_0,
		EquipmentSlot_1,
		EquipmentSlot_2,
		EquipmentSlot_3,
		EquipmentSlot_4,
		EquipmentSlot_5,
		EquipmentSlot_6,
		EquipmentSlot_7,
		EquipmentSlot_8,
		EquipmentSlot_9,
	};

	for (int32 SlotIndex = 0; SlotIndex < EquipmentSlots.Num(); ++SlotIndex)
	{
		ULSChipEquipmentSlotWidget* EquipmentSlot = EquipmentSlots[SlotIndex];
		if (!EquipmentSlot)
		{
			UE_LOG(LogLS, Warning, TEXT("EquipmentSlot_%d is not bound on %s."), SlotIndex, *GetNameSafe(this));
			continue;
		}

		EquipmentSlot->SetEquipmentSlotContext(this, SlotIndex);
		if (EquipmentItems.IsValidIndex(SlotIndex) && LSInventorySlotUtils::IsFilled(EquipmentItems[SlotIndex]))
		{
			EquipmentSlot->SetEquipmentItem(EquipmentItems[SlotIndex]);
		}
		else
		{
			EquipmentSlot->ClearEquipmentSlot();
		}
	}
}

bool ULSChipStationWidget::EquipChipToHardwareSlot(const ULSInventoryDragDropOperation& DragOperation, const int32 EquipmentSlotIndex)
{
	if (!DragOperation.SourceChipStationWidget || DragOperation.SourceChipStationWidget != this)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot equip chip because source chip station does not match on %s."), *GetNameSafe(this));
		return false;
	}

	UGameInstance* GameInstance = GetGameInstance();
	ULSSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
	if (!SaveSubsystem)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot equip chip because SaveSubsystem is missing on %s."), *GetNameSafe(this));
		return false;
	}

	const bool bEquipped = SaveSubsystem->EquipChipFromStoredSlot(
		DragOperation.SourceSlotArea,
		DragOperation.SourceSlotIndex,
		EquipmentSlotIndex);
	if (bEquipped)
	{
		RefreshChipSlots();
		RefreshEquipmentSlots();
	}

	return bEquipped;
}

bool ULSChipStationWidget::DropEquippedChipToHardwareSlot(const ULSInventoryDragDropOperation& DragOperation, const int32 TargetEquipmentSlotIndex)
{
	if (!DragOperation.SourceChipStationWidget || DragOperation.SourceChipStationWidget != this)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot drop equipped chip because source chip station does not match on %s."), *GetNameSafe(this));
		return false;
	}

	if (!DragOperation.SourceChipEquipmentSlotWidget || DragOperation.SourceEquipmentSlotIndex == INDEX_NONE)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot drop equipped chip because source equipment slot is invalid on %s."), *GetNameSafe(this));
		return false;
	}

	UGameInstance* GameInstance = GetGameInstance();
	ULSSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
	if (!SaveSubsystem)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot drop equipped chip because SaveSubsystem is missing on %s."), *GetNameSafe(this));
		return false;
	}

	const bool bDropped = SaveSubsystem->DropChipEquipmentSlot(
		DragOperation.SourceEquipmentSlotIndex,
		TargetEquipmentSlotIndex);
	if (bDropped)
	{
		RefreshChipSlots();
		RefreshEquipmentSlots();
	}

	return bDropped;
}

bool ULSChipStationWidget::UnequipChipToWarehouse(const ULSInventoryDragDropOperation& DragOperation)
{
	if (!DragOperation.SourceChipStationWidget || DragOperation.SourceChipStationWidget != this)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot unequip chip because source chip station does not match on %s."), *GetNameSafe(this));
		return false;
	}

	if (!DragOperation.SourceChipEquipmentSlotWidget || DragOperation.SourceEquipmentSlotIndex == INDEX_NONE)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot unequip chip because source equipment slot is invalid on %s."), *GetNameSafe(this));
		return false;
	}

	UGameInstance* GameInstance = GetGameInstance();
	ULSSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
	if (!SaveSubsystem)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot unequip chip because SaveSubsystem is missing on %s."), *GetNameSafe(this));
		return false;
	}

	const bool bUnequipped = SaveSubsystem->UnequipChipToWarehouse(DragOperation.SourceEquipmentSlotIndex);
	if (bUnequipped)
	{
		RefreshChipSlots();
		RefreshEquipmentSlots();
	}

	return bUnequipped;
}

bool ULSChipStationWidget::SwapEquippedChipWithStoredSlot(const ULSInventoryDragDropOperation& DragOperation, const ELSInventorySlotArea TargetArea, const int32 TargetSlotIndex)
{
	if (!DragOperation.SourceChipStationWidget || DragOperation.SourceChipStationWidget != this)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot swap equipped chip because source chip station does not match on %s."), *GetNameSafe(this));
		return false;
	}

	if (!DragOperation.SourceChipEquipmentSlotWidget || DragOperation.SourceEquipmentSlotIndex == INDEX_NONE)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot swap equipped chip because source equipment slot is invalid on %s."), *GetNameSafe(this));
		return false;
	}

	UGameInstance* GameInstance = GetGameInstance();
	ULSSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
	if (!SaveSubsystem)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot swap equipped chip because SaveSubsystem is missing on %s."), *GetNameSafe(this));
		return false;
	}

	const bool bSwapped = SaveSubsystem->EquipChipFromStoredSlot(
		TargetArea,
		TargetSlotIndex,
		DragOperation.SourceEquipmentSlotIndex);
	if (bSwapped)
	{
		RefreshChipSlots();
		RefreshEquipmentSlots();
	}

	return bSwapped;
}

void ULSChipStationWidget::InitializeEquipmentSlots()
{
	TArray<ULSChipEquipmentSlotWidget*> EquipmentSlots = {
		EquipmentSlot_0,
		EquipmentSlot_1,
		EquipmentSlot_2,
		EquipmentSlot_3,
		EquipmentSlot_4,
		EquipmentSlot_5,
		EquipmentSlot_6,
		EquipmentSlot_7,
		EquipmentSlot_8,
		EquipmentSlot_9,
	};

	for (int32 SlotIndex = 0; SlotIndex < EquipmentSlots.Num(); ++SlotIndex)
	{
		ULSChipEquipmentSlotWidget* EquipmentSlot = EquipmentSlots[SlotIndex];
		if (!EquipmentSlot)
		{
			UE_LOG(LogLS, Warning, TEXT("EquipmentSlot_%d is not bound on %s."), SlotIndex, *GetNameSafe(this));
			continue;
		}

		EquipmentSlot->SetEquipmentSlotContext(this, SlotIndex);
		EquipmentSlot->ClearEquipmentSlot();
	}
}

bool ULSChipStationWidget::IsPointerInsideChipSlotBorder(const FVector2D ScreenPosition) const
{
	if (!ChipSlotBorder)
	{
		UE_LOG(LogLS, Warning, TEXT("ChipSlotBorder is not bound on %s."), *GetNameSafe(this));
		return false;
	}

	const FGeometry& ChipSlotBorderGeometry = ChipSlotBorder->GetCachedGeometry();
	const FVector2D LocalPosition = ChipSlotBorderGeometry.AbsoluteToLocal(ScreenPosition);
	return LocalPosition.X >= 0.0f &&
		LocalPosition.Y >= 0.0f &&
		LocalPosition.X <= ChipSlotBorderGeometry.GetLocalSize().X &&
		LocalPosition.Y <= ChipSlotBorderGeometry.GetLocalSize().Y;
}

ULSItemSlotWidget* ULSChipStationWidget::CreateChipSlotWidget() const
{
	APlayerController* OwningPlayer = GetOwningPlayer();
	UWorld* World = GetWorld();
	if (!OwningPlayer && !World)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot create chip slot because owner/world is missing on %s."), *GetNameSafe(this));
		return nullptr;
	}

	ULSItemSlotWidget* SlotWidget = OwningPlayer
		? CreateWidget<ULSItemSlotWidget>(OwningPlayer, ItemSlotWidgetClass)
		: CreateWidget<ULSItemSlotWidget>(World, ItemSlotWidgetClass);
	if (!SlotWidget)
	{
		UE_LOG(LogLS, Warning, TEXT("Failed to create chip slot widget on %s."), *GetNameSafe(this));
	}

	return SlotWidget;
}

void ULSChipStationWidget::SetChipStat(FName StatKey, int32 StatValue, int32 SignalLoss)
{
	ULSChipStatWidget* StatWidget = GetStatWidget(StatKey);
	if (!StatWidget)
	{
		UE_LOG(LogLS, Warning, TEXT("[ChipStation] '%s' 칸이 null (BindWidget 안 됨)."), *StatKey.ToString());
		return;
	}

	UE_LOG(LogLS, Warning, TEXT("[ChipStation] SetChipStat '%s' -> %s (Stat=%d, Signal=%d)"),
	       *StatKey.ToString(), *GetNameSafe(StatWidget), StatValue, SignalLoss);
	StatWidget->SetStat(LSChipStats::GetChipStatLabel(StatKey), StatValue, SignalLoss);
}

void ULSChipStationWidget::SetProtocolWidget(ULSProtocolWidget* ProtocolWidget, const TCHAR* ProtocolName, const int32 Level, const int32 SynergyStage) const
{
	if (!ProtocolWidget)
	{
		UE_LOG(LogLS, Warning, TEXT("[ChipStation] %s is null (BindWidget 확인 필요)."), ProtocolName);
		return;
	}

	ProtocolWidget->SetProtocol(Level, SynergyStage);
}

ULSChipStatWidget* ULSChipStationWidget::GetStatWidget(FName StatKey) const
{
	static const TMap<FName, int32> KeyToIndex = {
		{ TEXT("Chip_Attack"),              0 },
		{ TEXT("Chip_Critical_Rate"),       1 },
		{ TEXT("Chip_Critical_Damage"),     2 },
		{ TEXT("Chip_Defense_Penetration"), 3 },
		{ TEXT("Chip_Health"),              4 },
		{ TEXT("Chip_Defense"),             5 },
		{ TEXT("Chip_Attack_Speed"),        6 },
		{ TEXT("Chip_Move_Speed"),          7 },
	};

	const int32* Index = KeyToIndex.Find(StatKey);
	if (!Index)
	{
		return nullptr;
	}

	switch (*Index)
	{
	case 0: return ChipStat_Attack;
	case 1: return ChipStat_CriticalRate;
	case 2: return ChipStat_CriticalDamage;
	case 3: return ChipStat_DefensePenetration;
	case 4: return ChipStat_Health;
	case 5: return ChipStat_Defense;
	case 6: return ChipStat_AttackSpeed;
	case 7: return ChipStat_MoveSpeed;
	default: return nullptr;
	}
}
