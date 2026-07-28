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
#include "Inventory/LSRaidInventoryComponent.h"
#include "Kismet/GameplayStatics.h"
#include "LostSignal.h"
#include "Session/LSSaveSubsystem.h"
#include "TimerManager.h"
#include "UI/ChipSystem/LSChipEquipmentSlotWidget.h"
#include "UI/ChipSystem/LSChipStatWidget.h"
#include "UI/Common/LSConfirmDialogWidget.h"
#include "UI/Inventory/LSInventoryDragDropOperation.h"
#include "UI/LSUILayer.h"
#include "UI/Inventory/LSItemSlotWidget.h"
#include "UI/Inventory/LSSlotWidgetSync.h"
#include "UI/Minimap/LSMinimapWidget.h"
#include "UI/Protocol/LSProtocolWidget.h"
#include "UI/Skill/LSSkillBarWidget.h"
#include "UI/Survival/LSSurvivalStatusWidget.h"
#include "UI/Noise/LSSoundDirectionIndicatorWidget.h"

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

int32 CalculateDisappearingSignalSlotIndex(const float SignalPercent)
{
	const float ClampedPercent = FMath::Clamp(SignalPercent, 0.0f, 1.0f);
	if (ClampedPercent <= 0.0f)
	{
		return INDEX_NONE;
	}

	return FMath::Clamp(FMath::FloorToInt((1.0f - ClampedPercent) * 10.0f + KINDA_SMALL_NUMBER), 0, 9);
}

float CalculateSignalSlotDisappearProgress(const float SignalPercent, const int32 SlotIndex)
{
	if (SlotIndex == INDEX_NONE)
	{
		return 0.0f;
	}

	const float ClampedPercent = FMath::Clamp(SignalPercent, 0.0f, 1.0f);
	const float SlotInactiveThreshold = 1.0f - (static_cast<float>(SlotIndex + 1) * 0.1f);
	return FMath::Clamp((ClampedPercent - SlotInactiveThreshold) / 0.1f, 0.0f, 1.0f);
}

int32 GetProtocolTotalByType(const FLSChipProtocolTotals& Totals, const ELSProtocolType ProtocolType)
{
	switch (ProtocolType)
	{
	case ELSProtocolType::Survival:
		return Totals.Survival;
	case ELSProtocolType::Carrying:
		return Totals.Carrying;
	case ELSProtocolType::Battle:
		return Totals.Battle;
	case ELSProtocolType::Navigation:
		return Totals.Navigation;
	default:
		return 0;
	}
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

// 슬롯 배열에서 현재 채워진 인덱스 집합을 만든다.
TSet<int32> BuildFilledSlotIndexSet(const TArray<FLSSessionItem>& Slots)
{
	TSet<int32> Filled;
	for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
	{
		if (LSInventorySlotUtils::IsFilled(Slots[SlotIndex]))
		{
			Filled.Add(SlotIndex);
		}
	}

	return Filled;
}

// AlreadyFilled에 없던, 새로 채워진 첫 슬롯 인덱스를 반환한다. 없으면 INDEX_NONE.
int32 FindFirstNewlyFilledIndex(const TArray<FLSSessionItem>& Slots, const TSet<int32>& AlreadyFilled)
{
	for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
	{
		if (LSInventorySlotUtils::IsFilled(Slots[SlotIndex]) && !AlreadyFilled.Contains(SlotIndex))
		{
			return SlotIndex;
		}
	}

	return INDEX_NONE;
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
	if (!SkillBar)
	{
		UE_LOG(LogLS, Warning, TEXT("SkillBar is not bound on %s."), *GetNameSafe(this));
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

	RefreshChipStation();
}

void ULSChipStationWidget::NativeDestruct()
{
	if (SignalSlider)
	{
		SignalSlider->OnValueChanged.RemoveDynamic(this, &ULSChipStationWidget::HandleSignalSliderValueChanged);
	}

	// 스테이션이 강제로 닫힐 때(범위 이탈 등) 떠 있던 용량 알림 다이얼로그가 뷰포트에 남지 않게 정리한다.
	if (ActiveConfirmDialog && ActiveConfirmDialog->IsInViewport())
	{
		ActiveConfirmDialog->Cancel();
	}
	ActiveConfirmDialog = nullptr;

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
	// 풀 리빌드 = 화면 세션 경계. 출처 기억을 비워 이후 해제는 "기억 없음" 규칙(인벤토리 우선)을 따른다.
	ChipOriginByEquipmentIndex.Reset();
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

	// 칩 리스트는 총 칩 개수(미장착 + 장착)만큼 고정 크기로 잡는다. 장착하면 그 칸이 빈 칸(hole)으로 남고,
	// 해제하면 빈 칸에 다시 채워지므로 스테이션 조작 중에는 위젯을 추가/삭제할 필요가 없다(InsertChipListSlot 참고).
	int32 EquippedChipCount = 0;
	for (const FLSSessionItem& EquipmentItem : SaveSubsystem->GetChipEquipmentSlots())
	{
		if (LSInventorySlotUtils::IsFilled(EquipmentItem))
		{
			++EquippedChipCount;
		}
	}
	const int32 TotalChipSlotCount = ViewItems.Num() + EquippedChipCount;

	LSSlotWidgetSync::SyncSlotWidgets(ChipSlotWrapBox, ItemSlotWidgetClass, GetOwningPlayer(), GetWorld(), TotalChipSlotCount,
		[this, &ViewItems](const int32 SlotIndex, ULSItemSlotWidget& SlotWidget)
		{
			SlotWidget.SetSlotLayoutSize(ChipItemSlotSize);

			if (!ViewItems.IsValidIndex(SlotIndex))
			{
				// 장착 칩 몫의 예비 빈 칸. 해제 시 InsertChipListSlot이 이 칸을 재사용한다.
				SlotWidget.SetChipStationSlotContext(this, ELSInventorySlotArea::Warehouse, INDEX_NONE, NAME_None, 0, {});
				SlotWidget.ClearItem();
				return;
			}

			const FLSChipStationViewItem& ViewItem = ViewItems[SlotIndex];
			SlotWidget.SetItem(ViewItem.Item.ItemRowName, ViewItem.Item.Amount, ViewItem.Item.ChipStats);
			SlotWidget.SetChipStationSlotContext(this, ViewItem.SourceArea, ViewItem.SourceSlotIndex, ViewItem.Item.ItemRowName, ViewItem.Item.Amount, ViewItem.Item.ChipStats);
		});
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

	// 프로토콜 표시는 디버그 오버라이드가 켜져 있으면 그 값을, 아니면 장착 칩 합산값(현재=활성칩, 이전=전체칩)을 따라간다.
	const TArray<FLSSessionItem> ActiveEquipmentItems = LSChipStats::BuildSignalActiveEquipmentItems(EquipmentItems, InactiveSlotCount);
	const FLSChipProtocolTotals ActiveProtocolTotals = LSChipStats::AggregateChipProtocolTotals(ActiveEquipmentItems, this);
	const FLSChipProtocolTotals AllProtocolTotals = LSChipStats::AggregateChipProtocolTotals(AllEquipmentItems, this);

	int32 SurvivalCurrent = 0, SurvivalPrevious = 0;
	int32 CarryingCurrent = 0, CarryingPrevious = 0;
	int32 BattleCurrent = 0, BattlePrevious = 0;
	int32 NavigationCurrent = 0, NavigationPrevious = 0;
	ResolveProtocolPreviewLevels(ELSProtocolType::Survival, ActiveProtocolTotals, AllProtocolTotals, SurvivalCurrent, SurvivalPrevious);
	ResolveProtocolPreviewLevels(ELSProtocolType::Carrying, ActiveProtocolTotals, AllProtocolTotals, CarryingCurrent, CarryingPrevious);
	ResolveProtocolPreviewLevels(ELSProtocolType::Battle, ActiveProtocolTotals, AllProtocolTotals, BattleCurrent, BattlePrevious);
	ResolveProtocolPreviewLevels(ELSProtocolType::Navigation, ActiveProtocolTotals, AllProtocolTotals, NavigationCurrent, NavigationPrevious);

	SetPreviewMinimapNavigationLevels(NavigationCurrent, NavigationPrevious);
	SetPreviewSurvivalStatus(SurvivalCurrent, SurvivalPrevious);
	SetPreviewSignalChip(EquipmentItems, GetSignalGaugePercent());
	SetPreviewBattleProtocol(BattleCurrent, BattlePrevious);
	SetProtocolWidget(Protocol_Survival, TEXT("Protocol_Survival"), ELSProtocolType::Survival, SurvivalCurrent, SurvivalPrevious);
	SetProtocolWidget(Protocol_Carrying, TEXT("Protocol_Carrying"), ELSProtocolType::Carrying, CarryingCurrent, CarryingPrevious);
	SetProtocolWidget(Protocol_Battle, TEXT("Protocol_Battle"), ELSProtocolType::Battle, BattleCurrent, BattlePrevious);
	SetProtocolWidget(Protocol_Navigation, TEXT("Protocol_Navigation"), ELSProtocolType::Navigation, NavigationCurrent, NavigationPrevious);
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

void ULSChipStationWidget::QueueRefreshEquippedChipState()
{
	// 빠른 장착 전용 경량 갱신. 칩 리스트(RefreshChipSlots=정렬+리빌드)는 건드리지 않고
	// 용량 보정 + 장착칸 + 요약만 다음 틱에 한 번으로 합쳐 갱신한다(쓸기 중 매 장착마다 풀 리빌드 방지).
	if (bPendingEquippedStateRefresh)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		HandleCarryingSlotCapacityChanged();
		RefreshEquipmentSlots();
		RefreshEquippedChipSummary();
		return;
	}

	bPendingEquippedStateRefresh = true;
	World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		bPendingEquippedStateRefresh = false;
		HandleCarryingSlotCapacityChanged();
		RefreshEquipmentSlots();
		RefreshEquippedChipSummary();
	}));
}

void ULSChipStationWidget::HandleCarryingSlotCapacityChanged()
{
	if (ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer()))
	{
		PlayerController->DropOverflowInventorySlotsToWorld(nullptr, FVector::ZeroVector);
		PlayerController->RefreshOpenLobbyStorageWidget();
		// 폰 전용 RebuildInventoryWidgetSlots 만 호출하면 폰이 없는 로비에서 인벤토리 위젯이 갱신되지 않는다.
		// 그러면 인벤토리에 있던 칩을 장착해 슬롯을 비운 뒤에도 인벤토리 위젯이 stale 상태로 남아,
		// 그 칸을 창고로 옮길 때 GetInventory()[Index] 가 비어 전송이 실패한다(간헐적 인벤토리→창고 실패).
		// PC 의 통합 경로는 폰이 있으면 폰을, 없으면 등록된 로비 인벤토리 위젯을 갱신한다.
		PlayerController->RefreshActiveInventoryWidget();
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
		PlayChipSound(ChipEquipSound, TEXT("ChipEquipSound"));
		RecordChipOrigin(EquipmentSlotIndex, DragOperation.SourceSlotArea, DragOperation.SourceSlotIndex);
		// 정렬은 스테이션을 다시 열 때만 한다. 드래그한 소스 리스트 칸만 제자리 갱신하고
		// (장착칸이 비어 있었으면 hole, 차 있어 스왑됐으면 돌아온 칩 표시) 장착칸·요약·용량만 경량 갱신한다.
		if (ULSItemSlotWidget* SourceSlotWidget = DragOperation.SourceSlotWidget)
		{
			SourceSlotWidget->RefreshChipStationSlotFromStored();
		}
		QueueRefreshEquippedChipState();
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
		// 장착칸끼리의 이동/교환이므로 출처 기억도 함께 옮긴다.
		MoveChipOriginRecord(DragOperation.SourceEquipmentSlotIndex, TargetEquipmentSlotIndex);
		// 장착칸끼리의 이동이라 칩 리스트는 그대로다. 정렬/리빌드 없이 장착칸·요약·용량만 경량 갱신한다.
		QueueRefreshEquippedChipState();
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

	return UnequipChipFromSlot(DragOperation.SourceEquipmentSlotIndex);
}

bool ULSChipStationWidget::SwapEquippedChipWithStoredSlot(const ULSInventoryDragDropOperation& DragOperation, ULSItemSlotWidget* TargetStoredSlotWidget, const ELSInventorySlotArea TargetArea, const int32 TargetSlotIndex)
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

	// 칩↔칩 교체(스왑)로 들어오는 칩의 적재(Carrying) 프로토콜이 더 낮으면 인벤토리 용량이 줄어 초과분이 월드로 드롭돼 사라진다.
	// 해제 차단과 동일한 정책으로, 로비에서만 스왑 자체를 막고 알림을 띄운다(레이드 중에는 기존 즉시 드롭 정책 유지).
	const ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer());
	const ULSRaidInventoryComponent* RaidInventory = PlayerController ? PlayerController->GetRaidInventoryComponent() : nullptr;
	const bool bRaidActive = RaidInventory && RaidInventory->IsRaidActive();
	if (!bRaidActive && SaveSubsystem->WouldSwapChipDropInventoryItems(TargetArea, TargetSlotIndex, DragOperation.SourceEquipmentSlotIndex))
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot swap equipped chip at slot %d because reduced carrying capacity would drop inventory items on %s."),
			DragOperation.SourceEquipmentSlotIndex,
			*GetNameSafe(this));
		ShowCapacityBlockedDialog(NSLOCTEXT("LSChipStation", "SwapBlockedByCapacity",
			"인벤토리 용량이 부족합니다. 이 칩으로 교체하면 아이템이 버려지므로, 먼저 인벤토리를 정리하세요."));
		return false;
	}

	const bool bSwapped = SaveSubsystem->EquipChipFromStoredSlot(
		TargetArea,
		TargetSlotIndex,
		DragOperation.SourceEquipmentSlotIndex);
	if (bSwapped)
	{
		PlayChipSound(ChipEquipSound, TEXT("ChipEquipSound"));
		// 밀려난 칩은 대상 저장 슬롯으로 갔으므로, 이 장착칸의 출처는 새로 들어온 칩의 저장 슬롯으로 덮어쓴다.
		RecordChipOrigin(DragOperation.SourceEquipmentSlotIndex, TargetArea, TargetSlotIndex);
		// 정렬은 스테이션을 다시 열 때만 한다. 드롭 대상 리스트 칸만 제자리 갱신한다(스왑으로 이 칸엔 방금 해제된 칩이 들어온다).
		if (TargetStoredSlotWidget)
		{
			TargetStoredSlotWidget->RefreshChipStationSlotFromStored();
		}
		QueueRefreshEquippedChipState();
	}

	return bSwapped;
}

bool ULSChipStationWidget::QuickEquipChipToFirstEmptyHardwareSlot(const ELSInventorySlotArea SourceArea, const int32 SourceSlotIndex)
{
	UGameInstance* GameInstance = GetGameInstance();
	ULSSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
	if (!SaveSubsystem)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot quick-equip chip because SaveSubsystem is missing on %s."), *GetNameSafe(this));
		return false;
	}

	// 장착 슬롯 수는 SaveSubsystem이 단일 출처다. 첫 빈 칸을 마지막 인덱스부터 역방향으로 찾는다.
	const TArray<FLSSessionItem>& EquipmentItems = SaveSubsystem->GetChipEquipmentSlots();
	int32 TargetEquipmentSlotIndex = INDEX_NONE;
	for (int32 SlotIndex = EquipmentItems.Num() - 1; SlotIndex >= 0; --SlotIndex)
	{
		if (!LSInventorySlotUtils::IsFilled(EquipmentItems[SlotIndex]))
		{
			TargetEquipmentSlotIndex = SlotIndex;
			break;
		}
	}

	if (TargetEquipmentSlotIndex == INDEX_NONE)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot quick-equip chip because every hardware slot is full on %s."), *GetNameSafe(this));
		return false;
	}

	const bool bEquipped = SaveSubsystem->EquipChipFromStoredSlot(SourceArea, SourceSlotIndex, TargetEquipmentSlotIndex);
	if (bEquipped)
	{
		PlayChipSound(ChipEquipSound, TEXT("ChipEquipSound"));
		RecordChipOrigin(TargetEquipmentSlotIndex, SourceArea, SourceSlotIndex);
		// 칩 리스트는 호출 측(소스 슬롯 위젯)이 ClearItem으로 그 칸만 비운다. 여기선 리스트를 재정렬/리빌드하지 않고
		// 장착칸·요약·용량만 경량 갱신한다(정렬은 스테이션을 다시 열 때만).
		QueueRefreshEquippedChipState();
	}

	return bEquipped;
}

bool ULSChipStationWidget::QuickUnequipEquippedChipToWarehouse(const int32 EquipmentSlotIndex)
{
	return UnequipChipFromSlot(EquipmentSlotIndex);
}

bool ULSChipStationWidget::UnequipChipFromSlot(const int32 EquipmentSlotIndex)
{
	UGameInstance* GameInstance = GetGameInstance();
	ULSSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
	if (!SaveSubsystem)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot unequip chip because SaveSubsystem is missing on %s."), *GetNameSafe(this));
		return false;
	}

	if (IsUnequipBlockedByCapacity(*SaveSubsystem, EquipmentSlotIndex))
	{
		return false;
	}

	// 출처가 인벤토리거나 기억이 없으면(재오픈 등) 인벤토리 복귀를 먼저 시도하고, 자리가 없으면 창고로 폴백한다.
	int32 PreferredInventoryIndex = INDEX_NONE;
	const bool bTryInventory = ShouldTryUnequipToInventory(EquipmentSlotIndex, PreferredInventoryIndex);
	if (bTryInventory && TryUnequipChipToInventory(*SaveSubsystem, EquipmentSlotIndex, PreferredInventoryIndex))
	{
		ChipOriginByEquipmentIndex.Remove(EquipmentSlotIndex);
		return true;
	}

	if (!UnequipChipToWarehouseWithListUpdate(*SaveSubsystem, EquipmentSlotIndex))
	{
		return false;
	}

	if (bTryInventory)
	{
		// 인벤토리 복귀를 의도했지만 자리가 없어 창고로 폴백했다 — 어디로 갔는지 사용자에게 알린다.
		ShowCapacityBlockedDialog(NSLOCTEXT("LSChipStation", "UnequipMovedToWarehouse",
			"인벤토리가 가득 차 해제한 칩을 창고로 보냈습니다."));
	}
	ChipOriginByEquipmentIndex.Remove(EquipmentSlotIndex);
	return true;
}

bool ULSChipStationWidget::IsUnequipBlockedByCapacity(ULSSaveSubsystem& SaveSubsystem, const int32 EquipmentSlotIndex)
{
	// 이 칩을 해제하면 적재(Carrying) 프로토콜이 낮아져 인벤토리 용량이 줄고, 넘치는 아이템(일반 인벤토리)이 월드로 드롭돼 사라진다.
	// 그 경우 해제 자체를 막고(아이템 손실 방지) 알림을 띄운다. 데이터 반영 전에 검사해야 한다.
	// 단, 이 차단은 로비 전용이다. 레이드 중에는 초과분을 그대로 월드에 버리는 기존 정책을 따른다(칩 스테이션은 로비 전용이지만
	// 규칙을 코드로 명시해 레이드에서 실수로 막히지 않게 한다). 보호 슬롯(Safe)은 드롭이 아니라 잠금이므로 이 판정 대상이 아니다.
	const ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer());
	const ULSRaidInventoryComponent* RaidInventory = PlayerController ? PlayerController->GetRaidInventoryComponent() : nullptr;
	const bool bRaidActive = RaidInventory && RaidInventory->IsRaidActive();
	if (!bRaidActive && SaveSubsystem.WouldUnequipChipDropInventoryItems(EquipmentSlotIndex))
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot unequip chip at slot %d because reduced carrying capacity would drop inventory items on %s."),
			EquipmentSlotIndex,
			*GetNameSafe(this));
		ShowCapacityBlockedDialog(NSLOCTEXT("LSChipStation", "UnequipBlockedByCapacity",
			"인벤토리 용량이 부족합니다. 이 칩을 해제하면 아이템이 버려지므로, 먼저 인벤토리를 정리하세요."));
		return true;
	}
	return false;
}

bool ULSChipStationWidget::ShouldTryUnequipToInventory(const int32 EquipmentSlotIndex, int32& OutPreferredInventoryIndex) const
{
	OutPreferredInventoryIndex = INDEX_NONE;
	const FLSChipOriginRecord* Origin = ChipOriginByEquipmentIndex.Find(EquipmentSlotIndex);
	if (!Origin)
	{
		// 출처 기억 없음(재오픈·풀 리빌드 후): 인벤토리 첫 빈 칸을 우선 시도한다.
		return true;
	}

	if (Origin->Area != ELSInventorySlotArea::Inventory)
	{
		// 창고 출신은 기존처럼 창고로 돌아간다(알림 없음).
		return false;
	}

	OutPreferredInventoryIndex = Origin->SlotIndex;
	return true;
}

bool ULSChipStationWidget::TryUnequipChipToInventory(ULSSaveSubsystem& SaveSubsystem, const int32 EquipmentSlotIndex, const int32 PreferredInventoryIndex)
{
	int32 PlacedSlotIndex = INDEX_NONE;
	if (!SaveSubsystem.UnequipChipToInventory(EquipmentSlotIndex, PreferredInventoryIndex, PlacedSlotIndex))
	{
		return false;
	}

	PlayChipSound(ChipUnequipSound, TEXT("ChipUnequipSound"));

	// 배치 인덱스를 정확히 알므로 창고 경로와 달리 diff 없이 그 칸을 칩 리스트 빈 칸에 바로 표시한다.
	const TArray<FLSSessionItem>& Inventory = SaveSubsystem.GetInventory();
	if (Inventory.IsValidIndex(PlacedSlotIndex))
	{
		InsertChipListSlot(Inventory[PlacedSlotIndex], ELSInventorySlotArea::Inventory, PlacedSlotIndex);
	}
	QueueRefreshEquippedChipState();
	return true;
}

bool ULSChipStationWidget::UnequipChipToWarehouseWithListUpdate(ULSSaveSubsystem& SaveSubsystem, const int32 EquipmentSlotIndex)
{
	// 해제 전 채워진 창고 인덱스를 기록해, 해제 후 새로 채워진 칸(=돌아온 칩의 위치)을 찾는다.
	const TSet<int32> FilledBeforeUnequip = BuildFilledSlotIndexSet(SaveSubsystem.GetWarehouseItems());
	if (!SaveSubsystem.UnequipChipToWarehouse(EquipmentSlotIndex))
	{
		return false;
	}

	PlayChipSound(ChipUnequipSound, TEXT("ChipUnequipSound"));

	// 칩 리스트는 재정렬/리빌드하지 않고, 돌아온 칩을 빈 칸(빠른 장착으로 생긴 hole) 또는 맨 뒤에 꽂는다.
	const TArray<FLSSessionItem>& WarehouseAfter = SaveSubsystem.GetWarehouseItems();
	const int32 ReturnedWarehouseIndex = FindFirstNewlyFilledIndex(WarehouseAfter, FilledBeforeUnequip);
	if (ReturnedWarehouseIndex != INDEX_NONE)
	{
		InsertChipListSlot(WarehouseAfter[ReturnedWarehouseIndex], ELSInventorySlotArea::Warehouse, ReturnedWarehouseIndex);
		QueueRefreshEquippedChipState();
	}
	else
	{
		// 기존 스택에 합쳐진 예외 케이스: 위치를 특정할 수 없으므로 안전하게 풀 새로고침(정렬 포함)한다.
		HandleCarryingSlotCapacityChanged();
		QueueRefreshChipStation();
	}

	return true;
}

void ULSChipStationWidget::RecordChipOrigin(const int32 EquipmentSlotIndex, const ELSInventorySlotArea Area, const int32 SlotIndex)
{
	ChipOriginByEquipmentIndex.Add(EquipmentSlotIndex, FLSChipOriginRecord{ Area, SlotIndex });
}

void ULSChipStationWidget::MoveChipOriginRecord(const int32 FromEquipmentSlotIndex, const int32 ToEquipmentSlotIndex)
{
	// 장착칸끼리 이동/교환: To에 레코드가 있으면(교환) 서로 바꾸고, 없으면(이동) 옮긴다.
	FLSChipOriginRecord FromRecord;
	const bool bHasFrom = ChipOriginByEquipmentIndex.RemoveAndCopyValue(FromEquipmentSlotIndex, FromRecord);
	FLSChipOriginRecord ToRecord;
	const bool bHasTo = ChipOriginByEquipmentIndex.RemoveAndCopyValue(ToEquipmentSlotIndex, ToRecord);
	if (bHasFrom)
	{
		ChipOriginByEquipmentIndex.Add(ToEquipmentSlotIndex, FromRecord);
	}
	if (bHasTo)
	{
		ChipOriginByEquipmentIndex.Add(FromEquipmentSlotIndex, ToRecord);
	}
}

void ULSChipStationWidget::ShowCapacityBlockedDialog(const FText& Message)
{
	// 이미 알림이 떠 있으면 중복 생성하지 않는다. (타이틀/세팅의 확인 다이얼로그와 동일 패턴)
	if (ActiveConfirmDialog && ActiveConfirmDialog->IsInViewport())
	{
		return;
	}

	if (!ConfirmDialogClass)
	{
		UE_LOG(LogLS, Warning, TEXT("[Chip] ConfirmDialogClass is not set on %s. Check WBP_ChipStation."), *GetNameSafe(this));
		return;
	}

	// 더블클릭/드래그드롭 제스처가 이 함수를 호출한다. 그 제스처의 마우스 Down은 이미 아이템 슬롯이 소비했으므로
	// 지금 다이얼로그를 띄우면 확인 버튼(DownAndUp)이 짝 Down을 못 받아 첫 클릭이 씹힌다.
	// 제스처가 끝난 다음 프레임에 띄워 첫 클릭이 온전한 Down+Up이 되게 한다.
	UWorld* World = GetWorld();
	if (!World)
	{
		PresentCapacityBlockedDialog(Message);
		return;
	}

	TWeakObjectPtr<ULSChipStationWidget> WeakThis(this);
	const FText CapturedMessage = Message;
	World->GetTimerManager().SetTimerForNextTick([WeakThis, CapturedMessage]()
	{
		if (ULSChipStationWidget* StrongThis = WeakThis.Get())
		{
			StrongThis->PresentCapacityBlockedDialog(CapturedMessage);
		}
	});
}

void ULSChipStationWidget::PresentCapacityBlockedDialog(const FText& Message)
{
	// 다음 틱 사이에 이미 알림이 떠 있거나 클래스가 사라졌을 수 있어 다시 확인한다.
	if (ActiveConfirmDialog && ActiveConfirmDialog->IsInViewport())
	{
		return;
	}

	if (!ConfirmDialogClass)
	{
		UE_LOG(LogLS, Warning, TEXT("[Chip] ConfirmDialogClass is not set on %s. Check WBP_ChipStation."), *GetNameSafe(this));
		return;
	}

	APlayerController* OwningPlayer = GetOwningPlayer();
	ULSConfirmDialogWidget* Dialog = OwningPlayer
		? CreateWidget<ULSConfirmDialogWidget>(OwningPlayer, ConfirmDialogClass)
		: CreateWidget<ULSConfirmDialogWidget>(this, ConfirmDialogClass);
	if (!Dialog)
	{
		UE_LOG(LogLS, Warning, TEXT("[Chip] Failed to create capacity block dialog on %s."), *GetNameSafe(this));
		return;
	}

	Dialog->SetMessage(Message);
	// 정보 알림이라 확인/취소/ESC 어느 쪽이든 그냥 닫힌다.
	Dialog->OnConfirmed.AddDynamic(this, &ULSChipStationWidget::HandleCapacityDialogClosed);
	Dialog->OnCancelled.AddDynamic(this, &ULSChipStationWidget::HandleCapacityDialogClosed);

	Dialog->AddToViewport(LSUILayer::ModalPanelDialog);
	ActiveConfirmDialog = Dialog;
}

bool ULSChipStationWidget::HasActiveConfirmDialog() const
{
	return ActiveConfirmDialog && ActiveConfirmDialog->IsInViewport();
}

void ULSChipStationWidget::HandleCapacityDialogClosed()
{
	// 다이얼로그는 스스로 뷰포트에서 제거되므로 참조만 비운다.
	ActiveConfirmDialog = nullptr;
}

void ULSChipStationWidget::InsertChipListSlot(const FLSSessionItem& Chip, const ELSInventorySlotArea SourceArea, const int32 SourceSlotIndex)
{
	if (!ChipSlotWrapBox)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot insert chip list slot because ChipSlotWrapBox is not bound on %s."), *GetNameSafe(this));
		return;
	}

	// 장착으로 비워진 슬롯(hole)을 앞에서부터 재사용한다. 칩 리스트는 총 칩 개수만큼 고정 크기로
	// 만들어지므로(RefreshChipSlots 참고) 장착 칩이 돌아올 빈 칸은 항상 존재한다.
	ULSItemSlotWidget* TargetSlot = nullptr;
	for (int32 ChildIndex = 0; ChildIndex < ChipSlotWrapBox->GetChildrenCount(); ++ChildIndex)
	{
		ULSItemSlotWidget* SlotWidget = Cast<ULSItemSlotWidget>(ChipSlotWrapBox->GetChildAt(ChildIndex));
		if (SlotWidget && !SlotWidget->HasItem())
		{
			TargetSlot = SlotWidget;
			break;
		}
	}

	// 고정 크기 계약이 깨진 예외 상황(외부에서 칩이 늘어난 채 리스트가 갱신 안 됨 등)이면 풀 새로고침으로 복구한다.
	if (!TargetSlot)
	{
		UE_LOG(LogLS, Warning, TEXT("No empty chip list slot to insert returned chip on %s. Falling back to full refresh."), *GetNameSafe(this));
		QueueRefreshChipStation();
		return;
	}

	TargetSlot->ResetTransientSlotState();
	TargetSlot->SetItem(Chip.ItemRowName, Chip.Amount, Chip.ChipStats);
	TargetSlot->SetChipStationSlotContext(this, SourceArea, SourceSlotIndex, Chip.ItemRowName, Chip.Amount, Chip.ChipStats);
	TargetSlot->RefreshHoverStateFromCursor();
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

	if (!SoundIndicator)
	{
		UE_LOG(LogLS, Warning, TEXT("SoundIndicator is not bound on %s."), *GetNameSafe(this));
		return;
	}

	if (CurrentSurvivalProtocol >= 5)
	{
		SoundIndicator->SetPreviewSoundDirectionAspectRatio(16.0f / 9.0f);
		SoundIndicator->ApplyPreviewSoundDirectionParameters();
	}
	else
	{
		SoundIndicator->HideSoundDirection();
	}
}

void ULSChipStationWidget::SetPreviewSignalChip(const TArray<FLSSessionItem>& EquipmentItems, const float SignalPercent)
{
	if (!SurvivalStatus)
	{
		UE_LOG(LogLS, Warning, TEXT("SurvivalStatus is not bound on %s."), *GetNameSafe(this));
		return;
	}

	const int32 DisappearingSlotIndex = CalculateDisappearingSignalSlotIndex(SignalPercent);
	const FLSSessionItem* DisappearingItem = EquipmentItems.IsValidIndex(DisappearingSlotIndex)
		? &EquipmentItems[DisappearingSlotIndex]
		: nullptr;
	if (!DisappearingItem || !LSInventorySlotUtils::IsFilled(*DisappearingItem))
	{
		SurvivalStatus->SetPreviewSignalChip(NAME_None, 0.0f);
		return;
	}

	const float DisappearProgress = CalculateSignalSlotDisappearProgress(SignalPercent, DisappearingSlotIndex);
	SurvivalStatus->SetPreviewSignalChip(DisappearingItem->ItemRowName, DisappearProgress);
}

void ULSChipStationWidget::SetPreviewBattleProtocol(const int32 CurrentBattleProtocol, const int32 PreviousBattleProtocol)
{
	if (!SkillBar)
	{
		UE_LOG(LogLS, Warning, TEXT("SkillBar is not bound on %s."), *GetNameSafe(this));
		return;
	}

	if (ALSPlayerCharacter* PlayerCharacter = Cast<ALSPlayerCharacter>(GetOwningPlayerPawn()))
	{
		SkillBar->InitializeSkillBar(PlayerCharacter->GetPlayerSkillComponent());
	}

	SkillBar->SetPreviewBattleProtocolLevels(CurrentBattleProtocol, PreviousBattleProtocol);
}

void ULSChipStationWidget::ResolveProtocolPreviewLevels(const ELSProtocolType ProtocolType, const FLSChipProtocolTotals& ActiveTotals, const FLSChipProtocolTotals& AllTotals, int32& OutCurrentLevel, int32& OutPreviousLevel) const
{
	// 프로토콜 디버그 패널이 떠 있고 오버라이드 값이 설정돼 있을 때만 그 값을 따라간다.
	// 패널이 닫혀 있으면 잔존 오버라이드는 무시하고 장착 칩 합산값을 쓴다.
	if (const ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer()))
	{
		if (PlayerController->IsProtocolDebugWidgetVisible() && PlayerController->HasProtocolTestLevel(ProtocolType))
		{
			OutCurrentLevel = PlayerController->GetProtocolTestLevel(ProtocolType);
			OutPreviousLevel = OutCurrentLevel;
			return;
		}
	}

	OutCurrentLevel = GetProtocolTotalByType(ActiveTotals, ProtocolType);
	OutPreviousLevel = GetProtocolTotalByType(AllTotals, ProtocolType);
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

void ULSChipStationWidget::PlayChipSound(USoundBase* Sound, const TCHAR* SoundPropertyName) const
{
	if (!Sound)
	{
		UE_LOG(LogLS, Warning, TEXT("%s is not set on %s."), SoundPropertyName, *GetNameSafe(this));
		return;
	}

	UGameplayStatics::PlaySound2D(this, Sound);
}

void ULSChipStationWidget::SetChipStat(FName StatKey, int32 StatValue, int32 SignalLoss)
{
	ULSChipStatWidget* StatWidget = GetStatWidget(StatKey);
	if (!StatWidget)
	{
		UE_LOG(LogLS, Warning, TEXT("[ChipStation] '%s' 칸이 null (BindWidget 안 됨)."), *StatKey.ToString());
		return;
	}

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
	// 칩 스테이션은 로비 전용 프리뷰다. 신호 게이지는 순수 프리뷰로만 동작하며 실제 저장 게이지/인벤토리 용량/전투 스탯(GAS)을
	// 바꾸지 않는다. 로비에서 게이지를 내려 적재 프로토콜이 줄면 인벤토리 초과분이 월드로 버려지므로(아이템 손실), 저장/용량 반영을
	// 하지 않고 칩 스테이션 자체 표시(활성·비활성 칩, 스탯·프로토콜 미리보기)만 갱신한다. 실제 게이지는 레이드에서만 변한다.
	SynchronizeSignalGauge(Percent);
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
	// 슬라이더는 순수 프리뷰다. 저장 게이지/인벤토리 용량/전투 스탯(GAS)에 반영하지 않고(로비 아이템 손실 방지)
	// 칩 스테이션 자체 표시(활성·비활성 칩, 스탯·프로토콜 미리보기)만 갱신한다. 실제 게이지는 레이드에서만 변한다.
	SynchronizeSignalGauge(Value);
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
