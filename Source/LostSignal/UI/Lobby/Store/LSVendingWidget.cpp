#include "UI/Lobby/Store/LSVendingWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/WrapBox.h"
#include "Data/LSDropSettings.h"
#include "Data/LSStoreStockRow.h"
#include "Inventory/LSInventorySlotUtils.h"
#include "LostSignal.h"
#include "Session/LSSaveSubsystem.h"
#include "TimerManager.h"
#include "UI/Inventory/LSItemSlotWidget.h"
#include "UI/Lobby/Store/LSVendingButtonWidget.h"
#include "UI/Lobby/Store/LSVendingSlotWidget.h"

#define LOCTEXT_NAMESPACE "LSStore"

void ULSVendingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 팝업의 F/X 단축키를 받으려면 포커스 대상이어야 한다.
	SetIsFocusable(true);

	const TCHAR* WidgetNames[] = {
		TEXT("GoldText"), TEXT("BackButton"), TEXT("RefreshTimerText"),
		TEXT("EquipTab"), TEXT("ConsumableTab"), TEXT("ChipTab"), TEXT("MaterialTab"), TEXT("StockBox"),
		TEXT("BagBox"), TEXT("SafeBox"), TEXT("WarehouseBox"),
		TEXT("BagCountText"), TEXT("SafeCountText"), TEXT("WarehouseCountText"),
		TEXT("DetailPanel"), TEXT("DetailItemSlot"), TEXT("DetailNameText"), TEXT("DetailStockText"), TEXT("DetailDescriptionText"),
		TEXT("TradeButton"), TEXT("TradeActionText"), TEXT("TradePriceText"),
		TEXT("DialogPanel"), TEXT("DialogItemSlot"), TEXT("DialogNameText"), TEXT("DialogPriceText"), TEXT("DialogQuantityText"),
		TEXT("DecreaseButton"), TEXT("IncreaseButton"), TEXT("YesButton"), TEXT("NoButton") };
	const UWidget* Widgets[] = {
		GoldText, BackButton, RefreshTimerText,
		EquipTab, ConsumableTab, ChipTab, MaterialTab, StockBox,
		BagBox, SafeBox, WarehouseBox,
		BagCountText, SafeCountText, WarehouseCountText,
		DetailPanel, DetailItemSlot, DetailNameText, DetailStockText, DetailDescriptionText,
		TradeButton, TradeActionText, TradePriceText,
		DialogPanel, DialogItemSlot, DialogNameText, DialogPriceText, DialogQuantityText,
		DecreaseButton, IncreaseButton, YesButton, NoButton };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Widgets); ++Index)
	{
		if (!Widgets[Index])
		{
			UE_LOG(LogLS, Warning, TEXT("[Store] %s is not bound on %s."), WidgetNames[Index], *GetNameSafe(this));
		}
	}
	if (!VendingSlotClass)
	{
		UE_LOG(LogLS, Warning, TEXT("[Store] VendingSlotClass is not set on %s."), *GetNameSafe(this));
	}

	if (BackButton) { BackButton->OnClicked.AddDynamic(this, &ULSVendingWidget::HandleBackClicked); }

	// 분류 버튼 라벨은 C++이 넣고, 클릭은 포인터 비교로 구분한다.
	ULSVendingButtonWidget* CategoryTabs[] = { EquipTab, ConsumableTab, ChipTab, MaterialTab };
	const FText CategoryLabels[] = {
		LOCTEXT("EquipCategory", "장비"), LOCTEXT("ConsumableCategory", "소모품"),
		LOCTEXT("ChipCategory", "칩"), LOCTEXT("MaterialCategory", "재료") };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(CategoryTabs); ++Index)
	{
		if (CategoryTabs[Index])
		{
			CategoryTabs[Index]->SetLabel(CategoryLabels[Index]);
			CategoryTabs[Index]->OnClicked.AddDynamic(this, &ULSVendingWidget::HandleCategoryTabClicked);
		}
	}

	if (TradeButton) { TradeButton->OnClicked.AddDynamic(this, &ULSVendingWidget::HandleTradeClicked); }
	if (DecreaseButton) { DecreaseButton->OnClicked.AddDynamic(this, &ULSVendingWidget::HandleDecreaseClicked); }
	if (IncreaseButton) { IncreaseButton->OnClicked.AddDynamic(this, &ULSVendingWidget::HandleIncreaseClicked); }
	if (YesButton) { YesButton->OnClicked.AddDynamic(this, &ULSVendingWidget::HandleYesClicked); }
	if (NoButton) { NoButton->OnClicked.AddDynamic(this, &ULSVendingWidget::HandleNoClicked); }

	if (ULSSaveSubsystem* SaveSubsystem = GetSaveSubsystem())
	{
		SaveSubsystem->OnGoldChanged.AddUObject(this, &ULSVendingWidget::HandleGoldChanged);
	}

	// 재고를 채우고 자동 새로고침 카운트다운을 시작한다. 위젯이 숨어 있어도 타이머는 계속 돈다.
	ResetStock();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(RefreshTimerHandle, this, &ULSVendingWidget::HandleRefreshTimerTick, 1.0f, true);
	}

	OpenVending();
}

void ULSVendingWidget::NativeDestruct()
{
	if (BackButton) { BackButton->OnClicked.RemoveDynamic(this, &ULSVendingWidget::HandleBackClicked); }
	ULSVendingButtonWidget* CategoryTabs[] = { EquipTab, ConsumableTab, ChipTab, MaterialTab };
	for (ULSVendingButtonWidget* CategoryTab : CategoryTabs)
	{
		if (CategoryTab)
		{
			CategoryTab->OnClicked.RemoveDynamic(this, &ULSVendingWidget::HandleCategoryTabClicked);
		}
	}
	if (TradeButton) { TradeButton->OnClicked.RemoveDynamic(this, &ULSVendingWidget::HandleTradeClicked); }
	if (DecreaseButton) { DecreaseButton->OnClicked.RemoveDynamic(this, &ULSVendingWidget::HandleDecreaseClicked); }
	if (IncreaseButton) { IncreaseButton->OnClicked.RemoveDynamic(this, &ULSVendingWidget::HandleIncreaseClicked); }
	if (YesButton) { YesButton->OnClicked.RemoveDynamic(this, &ULSVendingWidget::HandleYesClicked); }
	if (NoButton) { NoButton->OnClicked.RemoveDynamic(this, &ULSVendingWidget::HandleNoClicked); }

	if (ULSSaveSubsystem* SaveSubsystem = GetSaveSubsystem())
	{
		SaveSubsystem->OnGoldChanged.RemoveAll(this);
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RefreshTimerHandle);
	}

	Super::NativeDestruct();
}

void ULSVendingWidget::OpenVending()
{
	CloseQuantityDialog();
	ClearSelection();
	RefreshGoldText();
	RebuildStockList();
	RebuildOwnedPanels();
}

FReply ULSVendingWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (IsQuantityDialogOpen())
	{
		if (InKeyEvent.GetKey() == EKeys::F)
		{
			HandleYesClicked();
			return FReply::Handled();
		}
		if (InKeyEvent.GetKey() == EKeys::X)
		{
			HandleNoClicked();
			return FReply::Handled();
		}
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void ULSVendingWidget::HandleBackClicked()
{
	OnBackRequested.Broadcast();
}

void ULSVendingWidget::HandleCategoryTabClicked(ULSVendingButtonWidget* ClickedButton)
{
	if (ClickedButton == EquipTab) { SetCategory(ELSVendingCategory::Equip); }
	else if (ClickedButton == ConsumableTab) { SetCategory(ELSVendingCategory::Consumable); }
	else if (ClickedButton == ChipTab) { SetCategory(ELSVendingCategory::Chip); }
	else if (ClickedButton == MaterialTab) { SetCategory(ELSVendingCategory::Material); }
}

void ULSVendingWidget::SetCategory(const ELSVendingCategory NewCategory)
{
	CurrentCategory = NewCategory;
	RebuildStockList();
}

void ULSVendingWidget::HandleSlotClicked(ULSVendingSlotWidget* ClickedSlot)
{
	if (ClickedSlot)
	{
		SelectSlot(ClickedSlot);
	}
}

void ULSVendingWidget::HandleTradeClicked()
{
	if (!bHasSelection || IsQuantityDialogOpen())
	{
		return;
	}

	const ULSSaveSubsystem* SaveSubsystem = GetSaveSubsystem();
	if (!SaveSubsystem)
	{
		return;
	}

	int32 MaxQuantity = 0;
	if (bSelectedStock)
	{
		// 구매 최대 수량 = min(골드로 살 수 있는 개수, 남은 재고).
		const int32 Stock = GetCurrentStock(SelectedRowName);
		if (Stock <= 0)
		{
			UE_LOG(LogLS, Log, TEXT("[Store] %s is sold out."), *SelectedRowName.ToString());
			return;
		}
		const int32 Affordable = SelectedUnitPrice > 0 ? SaveSubsystem->GetGold() / SelectedUnitPrice : 0;
		MaxQuantity = FMath::Min(Affordable, Stock);
		if (MaxQuantity <= 0)
		{
			UE_LOG(LogLS, Log, TEXT("[Store] Not enough gold to buy %s (price %d, gold %d)."),
				*SelectedRowName.ToString(), SelectedUnitPrice, SaveSubsystem->GetGold());
			return;
		}
	}
	else
	{
		// 판매 최대 수량 = 그 슬롯에 들어 있는 개수.
		MaxQuantity = SelectedAmount;
	}

	OpenQuantityDialog(MaxQuantity);
}

void ULSVendingWidget::OpenQuantityDialog(const int32 InMaxQuantity)
{
	DialogMaxQuantity = FMath::Max(1, InMaxQuantity);
	DialogQuantity = 1;

	const LSInventorySlotUtils::FLSItemTradeInfo Info = LSInventorySlotUtils::ResolveItemTradeInfo(SelectedRowName);
	if (DialogItemSlot)
	{
		DialogItemSlot->SetDisplayOnlySlotContext();
		DialogItemSlot->SetItem(SelectedRowName, 1, TArray<FLSChipResolvedStat>());
	}
	if (DialogNameText)
	{
		DialogNameText->SetText(Info.Name);
	}
	if (DialogPriceText)
	{
		DialogPriceText->SetText(FText::AsNumber(SelectedUnitPrice));
	}
	RefreshDialogQuantityText();

	if (DialogPanel)
	{
		DialogPanel->SetVisibility(ESlateVisibility::Visible);
	}
	// F/X 키가 이 위젯으로 오도록 포커스를 가져온다.
	SetKeyboardFocus();
}

void ULSVendingWidget::CloseQuantityDialog() const
{
	if (DialogPanel)
	{
		DialogPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

bool ULSVendingWidget::IsQuantityDialogOpen() const
{
	return DialogPanel && DialogPanel->GetVisibility() != ESlateVisibility::Collapsed;
}

void ULSVendingWidget::RefreshDialogQuantityText() const
{
	if (DialogQuantityText)
	{
		DialogQuantityText->SetText(FText::Format(LOCTEXT("QuantityFormat", "{0}/{1}"),
			FText::AsNumber(DialogQuantity), FText::AsNumber(DialogMaxQuantity)));
	}
}

void ULSVendingWidget::HandleDecreaseClicked()
{
	DialogQuantity = FMath::Max(1, DialogQuantity - 1);
	RefreshDialogQuantityText();
}

void ULSVendingWidget::HandleIncreaseClicked()
{
	DialogQuantity = FMath::Min(DialogMaxQuantity, DialogQuantity + 1);
	RefreshDialogQuantityText();
}

void ULSVendingWidget::HandleYesClicked()
{
	if (!IsQuantityDialogOpen())
	{
		return;
	}
	CloseQuantityDialog();

	if (!bHasSelection || DialogQuantity <= 0)
	{
		return;
	}

	if (bSelectedStock)
	{
		ExecuteBuy(DialogQuantity);
	}
	else
	{
		ExecuteSell(DialogQuantity);
	}

	// 거래 후 내 아이템/선택 상태를 최신으로 되돌린다. 골드 표시는 OnGoldChanged 구독이 갱신한다.
	RebuildOwnedPanels();
	ClearSelection();
}

void ULSVendingWidget::HandleNoClicked()
{
	CloseQuantityDialog();
}

void ULSVendingWidget::HandleGoldChanged()
{
	RefreshGoldText();
}

void ULSVendingWidget::HandleRefreshTimerTick()
{
	RemainingRefreshSeconds -= 1.0f;
	if (RemainingRefreshSeconds <= 0.0f)
	{
		ResetStock();
		// 열려 있는 상세/팝업의 재고 표시가 낡았을 수 있으니 선택을 정리한다.
		CloseQuantityDialog();
		ClearSelection();
		UE_LOG(LogLS, Log, TEXT("[Store] Vending stock refreshed."));
		return;
	}
	UpdateRefreshTimerText();
}

void ULSVendingWidget::ResetStock()
{
	CurrentStockByRow.Reset();
	MaxStockByRow.Reset();

	const ULSDropSettings* Settings = GetDefault<ULSDropSettings>();
	UDataTable* StockTable = Settings ? Settings->StoreStockTable.LoadSynchronous() : nullptr;
	if (StockTable)
	{
		for (const FName RowName : StockTable->GetRowNames())
		{
			const FLSStoreStockRow* StockRow = StockTable->FindRow<FLSStoreStockRow>(RowName, TEXT("ResetStock"));
			if (StockRow && !StockRow->Item_Name.IsNone())
			{
				CurrentStockByRow.Add(StockRow->Item_Name, StockRow->Stock_Max);
				MaxStockByRow.Add(StockRow->Item_Name, StockRow->Stock_Max);
			}
		}
	}

	RemainingRefreshSeconds = FMath::Max(1.0f, RefreshIntervalSeconds);
	UpdateRefreshTimerText();
}

void ULSVendingWidget::UpdateRefreshTimerText() const
{
	if (!RefreshTimerText)
	{
		return;
	}

	const int32 TotalSeconds = FMath::Max(0, FMath::CeilToInt(RemainingRefreshSeconds));
	RefreshTimerText->SetText(FText::Format(LOCTEXT("RefreshTimerFormat", "{0} : {1} : {2}"),
		FText::FromString(FString::Printf(TEXT("%02d"), TotalSeconds / 3600)),
		FText::FromString(FString::Printf(TEXT("%02d"), (TotalSeconds / 60) % 60)),
		FText::FromString(FString::Printf(TEXT("%02d"), TotalSeconds % 60))));
}

int32 ULSVendingWidget::GetCurrentStock(const FName ItemRowName) const
{
	const int32* Found = CurrentStockByRow.Find(ItemRowName);
	return Found ? *Found : 0;
}

void ULSVendingWidget::RefreshGoldText() const
{
	const ULSSaveSubsystem* SaveSubsystem = GetSaveSubsystem();
	if (GoldText && SaveSubsystem)
	{
		GoldText->SetText(FText::AsNumber(SaveSubsystem->GetGold()));
	}
}

void ULSVendingWidget::RebuildStockList()
{
	if (!StockBox)
	{
		return;
	}
	StockBox->ClearChildren();

	const ULSDropSettings* Settings = GetDefault<ULSDropSettings>();
	UDataTable* StockTable = Settings ? Settings->StoreStockTable.LoadSynchronous() : nullptr;
	if (!StockTable)
	{
		UE_LOG(LogLS, Warning, TEXT("[Store] StoreStockTable is not set. Vending list will be empty."));
		return;
	}

	for (const FName RowName : StockTable->GetRowNames())
	{
		const FLSStoreStockRow* StockRow = StockTable->FindRow<FLSStoreStockRow>(RowName, TEXT("RebuildStockList"));
		if (!StockRow || StockRow->Item_Name.IsNone())
		{
			continue;
		}
		if (ResolveCategory(StockRow->Item_Name) != CurrentCategory)
		{
			continue;
		}

		const LSInventorySlotUtils::FLSItemTradeInfo Info = LSInventorySlotUtils::ResolveItemTradeInfo(StockRow->Item_Name);
		if (!Info.bValid)
		{
			continue;
		}

		if (ULSVendingSlotWidget* SlotWidget = CreateSlotWidget(StockBox))
		{
			SlotWidget->SetStockItem(StockRow->Item_Name, Info.Cost);
		}
	}
}

void ULSVendingWidget::RebuildOwnedPanels()
{
	const ULSSaveSubsystem* SaveSubsystem = GetSaveSubsystem();
	if (!SaveSubsystem)
	{
		return;
	}

	RebuildOwnedBox(BagBox, ELSInventorySlotArea::Inventory, SaveSubsystem->GetInventory(), SaveSubsystem->GetMaxInventorySlotCount(), BagCountText);
	RebuildOwnedBox(SafeBox, ELSInventorySlotArea::Safe, SaveSubsystem->GetSafeStash(), SaveSubsystem->GetMaxSafeStashSlotCount(), SafeCountText);
	RebuildOwnedBox(WarehouseBox, ELSInventorySlotArea::Warehouse, SaveSubsystem->GetWarehouseItems(), MaxWarehouseSlotCount, WarehouseCountText);
}

void ULSVendingWidget::RebuildOwnedBox(UWrapBox* TargetBox, const ELSInventorySlotArea Area, const TArray<FLSSessionItem>& Items, const int32 MaxSlotCount, UTextBlock* CountText)
{
	if (!TargetBox)
	{
		return;
	}
	TargetBox->ClearChildren();

	int32 FilledCount = 0;
	for (int32 SlotIndex = 0; SlotIndex < Items.Num(); ++SlotIndex)
	{
		if (!LSInventorySlotUtils::IsFilled(Items[SlotIndex]))
		{
			continue;
		}

		++FilledCount;
		if (ULSVendingSlotWidget* SlotWidget = CreateSlotWidget(TargetBox))
		{
			SlotWidget->SetOwnedItem(Items[SlotIndex].ItemRowName, Items[SlotIndex].Amount, Area, SlotIndex);
		}
	}

	if (CountText)
	{
		CountText->SetText(FText::Format(LOCTEXT("OwnedCountFormat", "{0}/{1}"),
			FText::AsNumber(FilledCount), FText::AsNumber(FMath::Max(0, MaxSlotCount))));
	}
}

ULSVendingSlotWidget* ULSVendingWidget::CreateSlotWidget(UWrapBox* TargetBox)
{
	if (!TargetBox || !VendingSlotClass)
	{
		return nullptr;
	}

	ULSVendingSlotWidget* SlotWidget = CreateWidget<ULSVendingSlotWidget>(this, VendingSlotClass);
	if (!SlotWidget)
	{
		UE_LOG(LogLS, Warning, TEXT("[Store] Failed to create vending slot on %s."), *GetNameSafe(this));
		return nullptr;
	}

	SlotWidget->OnClicked.AddDynamic(this, &ULSVendingWidget::HandleSlotClicked);
	TargetBox->AddChildToWrapBox(SlotWidget);
	return SlotWidget;
}

void ULSVendingWidget::SelectSlot(ULSVendingSlotWidget* SlotWidget)
{
	if (ULSVendingSlotWidget* Previous = SelectedSlotWidget.Get())
	{
		Previous->SetSelected(false);
	}

	const LSInventorySlotUtils::FLSItemTradeInfo Info = LSInventorySlotUtils::ResolveItemTradeInfo(SlotWidget->GetItemRowName());
	if (!Info.bValid)
	{
		ClearSelection();
		return;
	}

	bHasSelection = true;
	bSelectedStock = SlotWidget->IsStockSlot();
	SelectedRowName = SlotWidget->GetItemRowName();
	SelectedAmount = SlotWidget->GetAmount();
	SelectedArea = SlotWidget->GetArea();
	SelectedSlotIndex = SlotWidget->GetSlotIndex();
	// 구매가 = Item_Cost 그대로, 판매가 = Item_Cost * 비율(기획 확정 전 임시 규칙).
	SelectedUnitPrice = bSelectedStock ? Info.Cost : FMath::Max(0, FMath::RoundToInt(Info.Cost * SellPriceRatio));
	SelectedSlotWidget = SlotWidget;
	SlotWidget->SetSelected(true);

	if (DetailPanel)
	{
		DetailPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	if (DetailItemSlot)
	{
		DetailItemSlot->SetDisplayOnlySlotContext();
		DetailItemSlot->SetItem(SelectedRowName, bSelectedStock ? 1 : SelectedAmount, TArray<FLSChipResolvedStat>());
	}
	if (DetailNameText)
	{
		DetailNameText->SetText(Info.Name);
	}
	if (DetailStockText)
	{
		DetailStockText->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (bSelectedStock)
		{
			// 구매(자판기)는 남은 재고/최대 재고.
			const int32* MaxStock = MaxStockByRow.Find(SelectedRowName);
			DetailStockText->SetText(FText::Format(LOCTEXT("StockFormat", "재고 : {0}/{1}"),
				FText::AsNumber(GetCurrentStock(SelectedRowName)), FText::AsNumber(MaxStock ? *MaxStock : 0)));
		}
		else
		{
			// 판매(내 아이템)는 보유 수량.
			DetailStockText->SetText(FText::Format(LOCTEXT("OwnedStockFormat", "재고 : {0}"), FText::AsNumber(SelectedAmount)));
		}
	}
	if (DetailDescriptionText)
	{
		DetailDescriptionText->SetText(Info.Description);
	}
	if (TradeActionText)
	{
		TradeActionText->SetText(bSelectedStock ? LOCTEXT("Buy", "구매") : LOCTEXT("Sell", "판매"));
	}
	if (TradePriceText)
	{
		TradePriceText->SetText(FText::AsNumber(SelectedUnitPrice));
	}
}

void ULSVendingWidget::ClearSelection()
{
	if (ULSVendingSlotWidget* Previous = SelectedSlotWidget.Get())
	{
		Previous->SetSelected(false);
	}
	SelectedSlotWidget = nullptr;
	bHasSelection = false;
	SelectedRowName = NAME_None;
	SelectedUnitPrice = 0;
	SelectedAmount = 0;
	SelectedSlotIndex = INDEX_NONE;

	if (DetailPanel)
	{
		DetailPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ULSVendingWidget::ExecuteBuy(const int32 Quantity)
{
	ULSSaveSubsystem* SaveSubsystem = GetSaveSubsystem();
	if (!SaveSubsystem || SelectedUnitPrice <= 0)
	{
		return;
	}

	// 골드/재고 한도로 한 번 더 클램프한다(팝업이 열린 사이 값이 바뀔 수 있음).
	const int32 Affordable = SaveSubsystem->GetGold() / SelectedUnitPrice;
	const int32 BuyQuantity = FMath::Min3(Quantity, Affordable, GetCurrentStock(SelectedRowName));
	if (BuyQuantity <= 0)
	{
		UE_LOG(LogLS, Log, TEXT("[Store] Buy canceled. Not enough gold or stock for %s."), *SelectedRowName.ToString());
		return;
	}

	// 인벤토리에 먼저 넣고, 실제로 들어간 수량만큼만 골드를 차감한다(가방이 가득이면 부분 구매).
	FLSSessionItem RemainingItem;
	SaveSubsystem->TryAddToInventory(SelectedRowName, BuyQuantity, TArray<FLSChipResolvedStat>(), RemainingItem);
	const int32 RemainingAmount = LSInventorySlotUtils::IsFilled(RemainingItem) ? RemainingItem.Amount : 0;
	const int32 AddedAmount = BuyQuantity - RemainingAmount;
	if (AddedAmount <= 0)
	{
		UE_LOG(LogLS, Log, TEXT("[Store] Buy canceled. Inventory is full for %s."), *SelectedRowName.ToString());
		return;
	}

	if (!SaveSubsystem->TrySpendGold(AddedAmount * SelectedUnitPrice))
	{
		UE_LOG(LogLS, Warning, TEXT("[Store] Gold spend failed after adding items: %s x%d."), *SelectedRowName.ToString(), AddedAmount);
	}

	// 실제로 지급된 수량만큼 재고를 줄인다.
	CurrentStockByRow.Add(SelectedRowName, FMath::Max(0, GetCurrentStock(SelectedRowName) - AddedAmount));
	UE_LOG(LogLS, Log, TEXT("[Store] Bought %s x%d for %d gold. Stock left: %d"),
		*SelectedRowName.ToString(), AddedAmount, AddedAmount * SelectedUnitPrice, GetCurrentStock(SelectedRowName));
}

void ULSVendingWidget::ExecuteSell(const int32 Quantity)
{
	ULSSaveSubsystem* SaveSubsystem = GetSaveSubsystem();
	if (!SaveSubsystem)
	{
		return;
	}

	// 팝업이 열린 사이 슬롯이 바뀌었을 수 있으니 원본 슬롯을 다시 확인한다.
	FLSSessionItem SlotItem;
	if (!SaveSubsystem->GetStoredSlotItem(SelectedArea, SelectedSlotIndex, SlotItem)
		|| SlotItem.ItemRowName != SelectedRowName)
	{
		UE_LOG(LogLS, Warning, TEXT("[Store] Sell canceled. Slot changed: %s."), *SelectedRowName.ToString());
		return;
	}

	const int32 SellQuantity = FMath::Min(Quantity, SlotItem.Amount);
	if (SellQuantity <= 0)
	{
		return;
	}

	if (SellQuantity >= SlotItem.Amount)
	{
		SaveSubsystem->ClearStoredSlot(SelectedArea, SelectedSlotIndex);
	}
	else
	{
		FLSSessionItem NewItem = SlotItem;
		NewItem.Amount = SlotItem.Amount - SellQuantity;
		FLSSessionItem PreviousItem;
		SaveSubsystem->ReplaceStoredSlotItem(SelectedArea, SelectedSlotIndex, NewItem, PreviousItem);
	}

	SaveSubsystem->AddGold(SellQuantity * SelectedUnitPrice);
	UE_LOG(LogLS, Log, TEXT("[Store] Sold %s x%d for %d gold."), *SelectedRowName.ToString(), SellQuantity, SellQuantity * SelectedUnitPrice);
}

ULSSaveSubsystem* ULSVendingWidget::GetSaveSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
}

ELSVendingCategory ULSVendingWidget::ResolveCategory(const FName ItemRowName)
{
	const FString RowNameString = ItemRowName.ToString();
	if (RowNameString.StartsWith(TEXT("Weapon_")) || RowNameString.StartsWith(TEXT("Armor_")))
	{
		return ELSVendingCategory::Equip;
	}
	if (RowNameString.StartsWith(TEXT("Chip_")))
	{
		return ELSVendingCategory::Chip;
	}

	// Item_ 계열은 Item_Type으로 소모품(4~9)과 그 외(재료 등)를 나눈다.
	const LSInventorySlotUtils::FLSItemTradeInfo Info = LSInventorySlotUtils::ResolveItemTradeInfo(ItemRowName);
	if (Info.bValid && Info.ItemType >= 4 && Info.ItemType <= 9)
	{
		return ELSVendingCategory::Consumable;
	}
	return ELSVendingCategory::Material;
}

#undef LOCTEXT_NAMESPACE
