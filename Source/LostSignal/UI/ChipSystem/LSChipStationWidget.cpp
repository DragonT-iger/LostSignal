#include "UI/ChipSystem/LSChipStationWidget.h"

#include "Components/Border.h"
#include "Components/ProgressBar.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/WrapBox.h"
#include "Characters/LSPlayerCharacter.h"
#include "Core/LSPlayerControllerBase.h"
#include "Data/LSChipRow.h"
#include "Data/LSChipStats.h"
#include "Data/LSGameDataSubsystem.h"
#include "Data/LSDropSettings.h"
#include "Engine/DataTable.h"
#include "Inventory/LSInventorySlotUtils.h"
#include "LostSignal.h"
#include "Session/LSSaveSubsystem.h"
#include "TimerManager.h"
#include "UI/ChipSystem/LSChipEquipmentSlotWidget.h"
#include "UI/ChipSystem/LSChipStatWidget.h"
#include "UI/Inventory/LSInventoryDragDropOperation.h"
#include "UI/Inventory/LSItemSlotWidget.h"
#include "UI/Minimap/LSMinimapWidget.h"
#include "UI/Protocol/LSProtocolWidget.h"
#include "UI/Survival/LSSurvivalStatusWidget.h"

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
		UE_LOG(LogLS, Warning, TEXT("Cannot resolve chip station data because ChipTable is not set on %s."), *GetNameSafe(LogContext));
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

int32 ResolveChipMemoryCost(UDataTable* ChipTable, const FName ItemRowName, const UObject* LogContext)
{
	if (!ChipTable)
	{
		return 0;
	}

	const FLSChipRow* Row = ChipTable->FindRow<FLSChipRow>(ItemRowName, TEXT("ChipStationMemory"));
	if (!Row)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot resolve chip memory cost for '%s' on %s."), *ItemRowName.ToString(), *GetNameSafe(LogContext));
		return 0;
	}

	return Row->Item_MemoryCost;
}

int32 CalculateEquippedChipMemoryCost(const TArray<FLSSessionItem>& Items, UDataTable* ChipTable, const UObject* LogContext)
{
	int32 TotalMemory = 0;
	for (const FLSSessionItem& Item : Items)
	{
		if (!IsInventoryChipItem(Item))
		{
			continue;
		}

		TotalMemory += ResolveChipMemoryCost(ChipTable, Item.ItemRowName, LogContext);
	}

	return TotalMemory;
}

int32 GetChipSourceSortOrder(const ELSInventorySlotArea SourceArea)
{
	return SourceArea == ELSInventorySlotArea::Inventory ? 0 : 1;
}

int32 CalculateInactiveSignalSlotCount(const float SignalPercent)
{
	const float ClampedPercent = FMath::Clamp(SignalPercent, 0.0f, 1.0f);
	const float SignalPercent100 = ClampedPercent * 100.0f;
	return FMath::Clamp(FMath::FloorToInt((100.0f - SignalPercent100 + KINDA_SMALL_NUMBER) / 10.0f), 0, 10);
}

int32 CalculateTemporaryProtocolPreviewLevel(const float SignalPercent)
{
	const float ClampedPercent = FMath::Clamp(SignalPercent, 0.0f, 1.0f);
	return FMath::Clamp(FMath::FloorToInt(ClampedPercent * 10.0f), 0, 9);
}

TArray<FLSSessionItem> BuildInactiveSignalEquipmentItems(const TArray<FLSSessionItem>& Items, const int32 InactiveSlotCount)
{
	TArray<FLSSessionItem> InactiveItems;
	InactiveItems.Reserve(FMath::Min(Items.Num(), InactiveSlotCount));

	for (int32 SlotIndex = 0; SlotIndex < Items.Num() && SlotIndex < InactiveSlotCount; ++SlotIndex)
	{
		InactiveItems.Add(Items[SlotIndex]);
	}

	return InactiveItems;
}

int32 GetHalfSignalLossValue(const TMap<FName, int32>& Totals, const FName StatKey)
{
	const int32* Total = Totals.Find(StatKey);
	return Total ? FMath::RoundToInt(static_cast<float>(*Total) * 0.5f) : 0;
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

	if (Protocol_Survival)
	{
		Protocol_Survival->SetProtocolType(ELSProtocolType::Survival);
	}
	if (Protocol_Carrying)
	{
		Protocol_Carrying->SetProtocolType(ELSProtocolType::Carrying);
	}
	if (Protocol_Battle)
	{
		Protocol_Battle->SetProtocolType(ELSProtocolType::Battle);
	}
	if (Protocol_Navigation)
	{
		Protocol_Navigation->SetProtocolType(ELSProtocolType::Navigation);
	}
	if (!SurvivalStatus)
	{
		UE_LOG(LogLS, Warning, TEXT("SurvivalStatus is not bound on %s."), *GetNameSafe(this));
	}

	InitializeEquipmentSlots();

	UGameInstance* GameInstance = GetGameInstance();
	ULSSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
	const float SavedSignalPercent = SaveSubsystem ? SaveSubsystem->GetChipSignalGaugePercent() : GetSignalGaugePercent();
	SynchronizeSignalGauge(SavedSignalPercent);

	if (SignalSlider)
	{
		SignalSlider->OnValueChanged.RemoveDynamic(this, &ULSChipStationWidget::HandleSignalSliderValueChanged);
		SignalSlider->OnValueChanged.AddDynamic(this, &ULSChipStationWidget::HandleSignalSliderValueChanged);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("SignalSlider is not bound on %s."), *GetNameSafe(this));
	}

	UE_LOG(LogLS, Warning, TEXT("[ChipStation] NativeConstruct -> RefreshChipStation (this=%s)"), *GetNameSafe(this));
	RefreshChipStation();
}

void ULSChipStationWidget::NativeDestruct()
{
	if (SignalSlider)
	{
		SignalSlider->OnValueChanged.RemoveDynamic(this, &ULSChipStationWidget::HandleSignalSliderValueChanged);
	}

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

	RefreshChipSlots();
	RefreshEquipmentSlots();
	RefreshEquippedChipSummary();
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
	const int32 InactiveSlotCount = GetInactiveSignalSlotCount();
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
		EquipmentSlot->SetSignalActive(SlotIndex >= InactiveSlotCount);
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

void ULSChipStationWidget::RefreshEquippedChipSummary()
{
	UGameInstance* GameInstance = GetGameInstance();
	ULSSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
	if (!SaveSubsystem)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot refresh equipped chip summary because SaveSubsystem is missing on %s."), *GetNameSafe(this));
		return;
	}

	const TArray<FLSSessionItem>& EquipmentItems = SaveSubsystem->GetChipEquipmentSlots();
	const int32 EquippedMemory = CalculateEquippedChipMemoryCost(EquipmentItems, LoadChipTable(this), this);
	SetEquippedChipMemoryText(EquippedMemory);

	const int32 InactiveSlotCount = GetInactiveSignalSlotCount();
	const TArray<FLSSessionItem>& AllEquipmentItems = EquipmentItems;
	const TArray<FLSSessionItem> InactiveEquipmentItems = BuildInactiveSignalEquipmentItems(EquipmentItems, InactiveSlotCount);
	const TMap<FName, int32> StatTotals = LSChipStats::AggregateChipStatTotals(AllEquipmentItems);
	const TMap<FName, int32> SignalLossTotals = LSChipStats::AggregateChipStatTotals(InactiveEquipmentItems);
	auto GetStatTotal = [&StatTotals](const FName StatKey)
	{
		const int32* Total = StatTotals.Find(StatKey);
		return Total ? *Total : 0;
	};
	auto GetSignalLossTotal = [&SignalLossTotals](const FName StatKey)
	{
		return GetHalfSignalLossValue(SignalLossTotals, StatKey);
	};

	// 최종 적용 수치는 활성 슬롯 합산 + 신호 유실 보정값이다.
	// UI에서는 두 값을 StatValueText / SignalLossText로 분리해서 보여준다.
	SetChipStat(TEXT("Chip_Attack"), GetStatTotal(TEXT("Chip_Attack")), GetSignalLossTotal(TEXT("Chip_Attack")));
	SetChipStat(TEXT("Chip_Critical_Rate"), GetStatTotal(TEXT("Chip_Critical_Rate")), GetSignalLossTotal(TEXT("Chip_Critical_Rate")));
	SetChipStat(TEXT("Chip_Critical_Damage"), GetStatTotal(TEXT("Chip_Critical_Damage")), GetSignalLossTotal(TEXT("Chip_Critical_Damage")));
	SetChipStat(TEXT("Chip_Defense_Penetration"), GetStatTotal(TEXT("Chip_Defense_Penetration")), GetSignalLossTotal(TEXT("Chip_Defense_Penetration")));
	SetChipStat(TEXT("Chip_Health"), GetStatTotal(TEXT("Chip_Health")), GetSignalLossTotal(TEXT("Chip_Health")));
	SetChipStat(TEXT("Chip_Defense"), GetStatTotal(TEXT("Chip_Defense")), GetSignalLossTotal(TEXT("Chip_Defense")));
	SetChipStat(TEXT("Chip_Attack_Speed"), GetStatTotal(TEXT("Chip_Attack_Speed")), GetSignalLossTotal(TEXT("Chip_Attack_Speed")));
	SetChipStat(TEXT("Chip_Skill_Haste"), GetStatTotal(TEXT("Chip_Skill_Haste")), GetSignalLossTotal(TEXT("Chip_Skill_Haste")));
	SetChipStat(TEXT("Chip_Recovery"), GetStatTotal(TEXT("Chip_Recovery")), GetSignalLossTotal(TEXT("Chip_Recovery")));
	SetChipStat(TEXT("Chip_Move_Speed"), GetStatTotal(TEXT("Chip_Move_Speed")), GetSignalLossTotal(TEXT("Chip_Move_Speed")));

	const int32 TemporaryProtocolLevel = CalculateTemporaryProtocolPreviewLevel(GetSignalGaugePercent());
	SetPreviewMinimapNavigationLevels(TemporaryProtocolLevel, TemporaryProtocolLevel);
	SetPreviewSurvivalStatus(TemporaryProtocolLevel, TemporaryProtocolLevel);
	SetProtocolWidget(Protocol_Survival, TEXT("Protocol_Survival"), ELSProtocolType::Survival, TemporaryProtocolLevel, TemporaryProtocolLevel);
	SetProtocolWidget(Protocol_Carrying, TEXT("Protocol_Carrying"), ELSProtocolType::Carrying, TemporaryProtocolLevel, TemporaryProtocolLevel);
	SetProtocolWidget(Protocol_Battle, TEXT("Protocol_Battle"), ELSProtocolType::Battle, TemporaryProtocolLevel, TemporaryProtocolLevel);
	SetProtocolWidget(Protocol_Navigation, TEXT("Protocol_Navigation"), ELSProtocolType::Navigation, TemporaryProtocolLevel, TemporaryProtocolLevel);
}

void ULSChipStationWidget::QueueRefreshChipStation()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		RefreshChipStation();
		return;
	}

	World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		RefreshChipStation();
	}));
}

void ULSChipStationWidget::HandleCarryingSlotCapacityChanged()
{
	if (ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer()))
	{
		PlayerController->DropOverflowInventorySlotsToWorld(nullptr, FVector::ZeroVector);
		PlayerController->RefreshOpenLobbyStorageWidget();
	}

	if (ALSPlayerCharacter* PlayerCharacter = Cast<ALSPlayerCharacter>(GetOwningPlayerPawn()))
	{
		PlayerCharacter->RebuildInventoryWidgetSlots();
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
		HandleCarryingSlotCapacityChanged();
		QueueRefreshChipStation();
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
		HandleCarryingSlotCapacityChanged();
		QueueRefreshChipStation();
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
		HandleCarryingSlotCapacityChanged();
		QueueRefreshChipStation();
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
		HandleCarryingSlotCapacityChanged();
		QueueRefreshChipStation();
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

void ULSChipStationWidget::SetPreviewMinimapNavigationLevels(const int32 CurrentNavigationProtocol, const int32 PreviousNavigationProtocol)
{
	if (!Minimap)
	{
		UE_LOG(LogLS, Warning, TEXT("Minimap is not bound on %s."), *GetNameSafe(this));
		return;
	}

	Minimap->SetPreviewNavigationLevels(CurrentNavigationProtocol, PreviousNavigationProtocol);
}

void ULSChipStationWidget::SetPreviewSurvivalStatus(const int32 CurrentSurvivalProtocol, const int32 PreviousSurvivalProtocol)
{
	if (!SurvivalStatus)
	{
		UE_LOG(LogLS, Warning, TEXT("SurvivalStatus is not bound on %s."), *GetNameSafe(this));
		return;
	}

	const float MaxHealth = 1000.0f;
	const float MaxStamina = 100.0f;
	const float CurrentHealth = 720.0f;
	SurvivalStatus->SetPreviewSurvivalStatus(
		CurrentSurvivalProtocol,
		PreviousSurvivalProtocol,
		CurrentHealth,
		MaxHealth,
		65.0f,
		MaxStamina);
	SurvivalStatus->SetHealthPreview(860.0f, 0.0f, true);
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

void ULSChipStationWidget::SetEquippedChipMemoryText(const int32 CurrentMemory)
{
	if (!MemoryText)
	{
		UE_LOG(LogLS, Warning, TEXT("MemoryText is not bound on %s."), *GetNameSafe(this));
		return;
	}

	MemoryText->SetText(FText::Format(
		NSLOCTEXT("LSChipStation", "ChipMemoryUsageFormat", "{0}/{1}"),
		FText::AsNumber(CurrentMemory),
		FText::AsNumber(MaxChipMemory)));
}

void ULSChipStationWidget::SetSignalGaugePercent(const float Percent)
{
	SynchronizeSignalGauge(Percent);

	UGameInstance* GameInstance = GetGameInstance();
	ULSSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
	if (SaveSubsystem)
	{
		SaveSubsystem->SetChipSignalGaugePercent(GetSignalGaugePercent());
		HandleCarryingSlotCapacityChanged();
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot save chip signal gauge because SaveSubsystem is missing on %s."), *GetNameSafe(this));
	}

	RefreshEquippedChipSummary();
	RefreshEquipmentSlots();
}

void ULSChipStationWidget::SynchronizeSignalGauge(const float Percent)
{
	const float ClampedPercent = FMath::Clamp(Percent, 0.0f, 1.0f);

	if (SignalProgressBar)
	{
		SignalProgressBar->SetPercent(ClampedPercent);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("SignalProgressBar is not bound on %s."), *GetNameSafe(this));
	}

	if (SignalSlider && !FMath::IsNearlyEqual(SignalSlider->GetValue(), ClampedPercent))
	{
		SignalSlider->SetValue(ClampedPercent);
	}
}

void ULSChipStationWidget::HandleSignalSliderValueChanged(const float Value)
{
	SynchronizeSignalGauge(Value);

	UGameInstance* GameInstance = GetGameInstance();
	ULSSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
	if (SaveSubsystem)
	{
		SaveSubsystem->SetChipSignalGaugePercent(GetSignalGaugePercent());
		HandleCarryingSlotCapacityChanged();
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot save chip signal gauge because SaveSubsystem is missing on %s."), *GetNameSafe(this));
	}

	RefreshEquippedChipSummary();
	RefreshEquipmentSlots();
}

float ULSChipStationWidget::GetSignalGaugePercent() const
{
	if (SignalSlider)
	{
		return SignalSlider->GetValue();
	}

	return SignalProgressBar ? SignalProgressBar->GetPercent() : 1.0f;
}

int32 ULSChipStationWidget::GetInactiveSignalSlotCount() const
{
	return CalculateInactiveSignalSlotCount(GetSignalGaugePercent());
}

void ULSChipStationWidget::SetProtocolWidget(ULSProtocolWidget* ProtocolWidget, const TCHAR* ProtocolName, const ELSProtocolType ProtocolType, const int32 CurrentLevel, const int32 PreviousLevel) const
{
	if (!ProtocolWidget)
	{
		UE_LOG(LogLS, Warning, TEXT("[ChipStation] %s is null (BindWidget 확인 필요)."), ProtocolName);
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	const ULSGameDataSubsystem* GameDataSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSGameDataSubsystem>() : nullptr;
	TArray<int32> StageLevels;
	if (GameDataSubsystem)
	{
		GameDataSubsystem->GetProtocolRequiredLevels(ProtocolType, StageLevels, TEXT("ChipStation"));
	}

	if (!StageLevels.IsEmpty())
	{
		ProtocolWidget->SetProtocolStageLevels(StageLevels);
	}
	ProtocolWidget->SetProtocolLevels(CurrentLevel, PreviousLevel, CurrentLevel);
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
		{ TEXT("Chip_Skill_Haste"),         7 },
		{ TEXT("Chip_Recovery"),            8 },
		{ TEXT("Chip_Move_Speed"),          9 },
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
	case 7: return ChipStat_Skill_Haste;
	case 8: return ChipStat_Recovery;
	case 9: return ChipStat_MoveSpeed;
	default: return nullptr;
	}
}
