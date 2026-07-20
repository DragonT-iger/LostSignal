#include "UI/Lobby/Store/LSVendingSlotWidget.h"

#include "Blueprint/DragDropOperation.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "LostSignal.h"
#include "UI/Inventory/LSItemSlotWidget.h"

void ULSVendingSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	const TCHAR* WidgetNames[] = { TEXT("SlotBorder"), TEXT("ItemSlot"), TEXT("PriceBox"), TEXT("PriceText") };
	const UWidget* Widgets[] = { SlotBorder, ItemSlot, PriceBox, PriceText };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Widgets); ++Index)
	{
		if (!Widgets[Index])
		{
			UE_LOG(LogLS, Warning, TEXT("[Store] %s is not bound on %s."), WidgetNames[Index], *GetNameSafe(this));
		}
	}

	if (SlotBorder)
	{
		NormalBorderColor = SlotBorder->GetBrushColor();
	}
}

FReply ULSVendingSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 빈 칸은 선택 대상이 아니다.
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && !ItemRowName.IsNone())
	{
		OnClicked.Broadcast(this);

		// 내 아이템 칸이면 드래그도 무장한다(움직이면 NativeOnDragDetected 발화, 그냥 떼면 선택만 남는다).
		if (CanDragFrom())
		{
			return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
		}
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void ULSVendingSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	if (!CanDragFrom())
	{
		return;
	}

	UDragDropOperation* DragOperation = NewObject<UDragDropOperation>(this);
	DragOperation->Payload = this;
	DragOperation->Pivot = EDragPivot::MouseDown;

	// 패키지 빌드에서 DefaultDragVisual=this는 입력을 죽인다(Docs/Troubleshooting/UIDragDropPackagedBuild.md).
	// 커서를 따라갈 별도 인스턴스를 만들어 쓴다.
	if (ULSVendingSlotWidget* DragVisual = CreateWidget<ULSVendingSlotWidget>(GetOwningPlayer(), GetClass()))
	{
		DragVisual->SetOwnedItem(ItemRowName, Amount, Area, SlotIndex);
		DragOperation->DefaultDragVisual = DragVisual;
	}

	OutOperation = DragOperation;
}

void ULSVendingSlotWidget::NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragEnter(InGeometry, InDragDropEvent, InOperation);

	if (CanAcceptDrop(InOperation))
	{
		bIsDragHover = true;
		ApplyBorderColor();
	}
}

void ULSVendingSlotWidget::NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragLeave(InDragDropEvent, InOperation);

	bIsDragHover = false;
	ApplyBorderColor();
}

bool ULSVendingSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	bIsDragHover = false;
	ApplyBorderColor();

	if (!CanAcceptDrop(InOperation))
	{
		return false;
	}

	ULSVendingSlotWidget* SourceSlot = Cast<ULSVendingSlotWidget>(InOperation->Payload);
	if (SourceSlot == this)
	{
		return false;
	}

	OnDropped.Broadcast(SourceSlot, this);
	return true;
}

bool ULSVendingSlotWidget::CanDragFrom() const
{
	return !bStockSlot && !ItemRowName.IsNone();
}

bool ULSVendingSlotWidget::CanAcceptDrop(const UDragDropOperation* InOperation) const
{
	// 출발지가 내 아이템 칸이고, 도착지도 내 영역(빈 칸 포함)이어야 한다. 자판기 상품 칸은 제외.
	const ULSVendingSlotWidget* SourceSlot = InOperation ? Cast<ULSVendingSlotWidget>(InOperation->Payload) : nullptr;
	return SourceSlot && SourceSlot->CanDragFrom() && !bStockSlot && SlotIndex != INDEX_NONE;
}

void ULSVendingSlotWidget::ResetHighlightState()
{
	bIsSelected = false;
	bIsDragHover = false;
	ApplyBorderColor();
}

void ULSVendingSlotWidget::SetStockItem(const FName InItemRowName, const int32 InPrice)
{
	ResetHighlightState();
	ItemRowName = InItemRowName;
	Price = InPrice;
	Amount = 1;
	bStockSlot = true;
	SlotIndex = INDEX_NONE;

	if (ItemSlot)
	{
		ItemSlot->SetDisplayOnlySlotContext();
		ItemSlot->SetItem(InItemRowName, 1, TArray<FLSChipResolvedStat>());
	}
	if (PriceBox)
	{
		PriceBox->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (PriceText)
	{
		PriceText->SetText(FText::AsNumber(InPrice));
	}
}

void ULSVendingSlotWidget::SetOwnedItem(const FName InItemRowName, const int32 InAmount, const ELSInventorySlotArea InArea, const int32 InSlotIndex)
{
	ResetHighlightState();
	ItemRowName = InItemRowName;
	Price = 0;
	Amount = InAmount;
	bStockSlot = false;
	Area = InArea;
	SlotIndex = InSlotIndex;

	if (ItemSlot)
	{
		ItemSlot->SetDisplayOnlySlotContext();
		ItemSlot->SetItem(InItemRowName, InAmount, TArray<FLSChipResolvedStat>());
	}
	if (PriceBox)
	{
		PriceBox->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ULSVendingSlotWidget::SetOwnedEmpty(const ELSInventorySlotArea InArea, const int32 InSlotIndex)
{
	ResetHighlightState();
	ItemRowName = NAME_None;
	Price = 0;
	Amount = 0;
	bStockSlot = false;
	Area = InArea;
	SlotIndex = InSlotIndex;

	if (ItemSlot)
	{
		ItemSlot->SetDisplayOnlySlotContext();
		ItemSlot->ClearItem();
	}
	if (PriceBox)
	{
		PriceBox->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ULSVendingSlotWidget::SetSelected(const bool bInSelected)
{
	bIsSelected = bInSelected;
	ApplyBorderColor();
}

void ULSVendingSlotWidget::ApplyBorderColor() const
{
	if (SlotBorder)
	{
		SlotBorder->SetBrushColor((bIsSelected || bIsDragHover) ? SelectedBorderColor : NormalBorderColor);
	}
}
