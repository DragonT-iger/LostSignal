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
// 미공개 placeholder(미확인 아이콘)가 적용된 상태를 나타내는 예약 키.
const FName PlaceholderIconKey(TEXT("__LSUnconfirmed__"));
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

	// 미공개 placeholder였다가 실제 아이템이 들어오는 전환이면 등장 pop-in을 재생한다.
	// 아래 본문(ApplyHoverVisual 등)이 정상 아이템으로 동작하도록 플래그를 먼저 끈다.
	const bool bWasPlaceholder = bIsPlaceholder;
	bIsPlaceholder = false;

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

	CurrentGradeBackgroundColor = ResolveGradeBackgroundColor(ItemRowName);

	ApplyHoverVisual();
	ItemIconImage->SetVisibility(ESlateVisibility::Visible);
	const bool bShouldShowAmount = Amount > 0 && LSInventorySlotUtils::ResolveItemMaxStack(ItemRowName, TEXT("ULSItemSlotWidget::SetItem")) > 1;
	AmountText->SetText(bShouldShowAmount ? FText::AsNumber(Amount) : FText::GetEmpty());
	AmountText->SetVisibility(bShouldShowAmount ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	SetTooltipItem(ItemRowName, Amount, ChipStats);
	DragItemRowName = ItemRowName;
	DragAmount = Amount;
	DragChipStats = ChipStats;
	bHasItem = true;

	// placeholder → 아이템 전환에서만 등장 pop-in을 시작한다. (인벤토리 등 일반 SetItem은 팝인 없음)
	if (bWasPlaceholder)
	{
		bIsPopInAnimating = true;
		PopInElapsed = 0.f;
	}
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

	// 배경은 SlotBackgroundImage가 항상 표시하므로 빈 슬롯에서는 아이콘만 숨긴다.
	DisplayedIconKey = EmptySlotIconKey;
	// 빈 슬롯은 등급색을 쓰지 않고 전용 빈 슬롯 배경색으로 되돌린다.
	CurrentGradeBackgroundColor = EmptySlotBackgroundColor;
	// placeholder/등장 연출 상태도 정리한다(공개됐다가 looted된 빈 칸, 풀 재사용 등).
	bIsPlaceholder = false;
	bIsPopInAnimating = false;

	ApplyHoverVisual();
	ItemIconImage->SetVisibility(ESlateVisibility::Collapsed);
	AmountText->SetText(FText::GetEmpty());
	AmountText->SetVisibility(ESlateVisibility::Collapsed);
	bHasItem = false;
	DragItemRowName = NAME_None;
	DragAmount = 0;
	DragChipStats.Reset();
	ClearTooltipItem();
}

void ULSItemSlotWidget::SetPlaceholder()
{
	if (!ItemIconImage)
	{
		UE_LOG(LogLS, Warning, TEXT("ItemIconImage is not bound on %s."), *GetNameSafe(this));
		return;
	}

	bIsPlaceholder = true;
	bIsPopInAnimating = false;
	bHasItem = false;
	DragItemRowName = NAME_None;
	DragAmount = 0;
	DragChipStats.Reset();
	ClearTooltipItem();

	// 미공개이므로 등급색을 쓰지 않고 빈 슬롯 배경색을 유지(슬롯 프레임은 항상 또렷).
	CurrentGradeBackgroundColor = EmptySlotBackgroundColor;
	if (SlotBackgroundImage)
	{
		SlotBackgroundImage->SetColorAndOpacity(CurrentGradeBackgroundColor);
	}

	// 미확인 아이콘 브러시 적용(미설정 시 기본 슬롯 텍스처로 폴백).
	if (DisplayedIconKey != PlaceholderIconKey)
	{
		UTexture2D* IconTexture = UnconfirmedIconTexture ? UnconfirmedIconTexture.Get() : DefaultSlotTexture.Get();
		if (!IconTexture)
		{
			UE_LOG(LogLS, Warning, TEXT("UnconfirmedIconTexture/DefaultSlotTexture not set for loot placeholder on %s."), *GetNameSafe(this));
		}
		else
		{
			ItemIconImage->SetBrushFromTexture(IconTexture);
		}
		DisplayedIconKey = PlaceholderIconKey;
	}
	ItemIconImage->SetVisibility(ESlateVisibility::Visible);

	// 펄스 시작값(다음 NativeTick부터 sin으로 갱신).
	ItemIconImage->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, PlaceholderPulseMinAlpha));

	if (AmountText)
	{
		AmountText->SetText(FText::GetEmpty());
		AmountText->SetVisibility(ESlateVisibility::Collapsed);
	}

	// pop-in/호버 강조와 무관하게 슬롯 자체는 정상 크기·불투명(아이콘만 펄스).
	SetRenderScale(FVector2D::UnitVector);
	SetRenderOpacity(1.f);
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

	ApplySlotBackground();

	if (!bHasItem)
	{
		ClearItem();
	}
}

void ULSItemSlotWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 루트 단계 공개 연출 외에는 비용이 들지 않도록 조기 종료한다(모든 슬롯이 공유하는 클래스).
	if (!bIsPopInAnimating && !bIsPlaceholder)
	{
		return;
	}

	// 등장 pop-in: 스케일(PopInStartScale→1.0) + 페이드(0→1). LSChipStatWidget과 동일한 이징.
	if (bIsPopInAnimating)
	{
		PopInElapsed += InDeltaTime;
		const float Duration = FMath::Max(KINDA_SMALL_NUMBER, PopInDuration);
		const float LinearAlpha = FMath::Clamp(PopInElapsed / Duration, 0.f, 1.f);
		const float Alpha = FMath::InterpEaseInOut(0.f, 1.f, LinearAlpha, 2.f);

		const float Scale = FMath::Lerp(PopInStartScale, 1.f, Alpha);
		SetRenderScale(FVector2D(Scale, Scale));
		SetRenderOpacity(Alpha);

		if (LinearAlpha >= 1.f)
		{
			SetRenderScale(FVector2D::UnitVector);
			SetRenderOpacity(1.f);
			bIsPopInAnimating = false;
		}
		return;
	}

	// 미공개 placeholder 펄스: 미확인 아이콘 알파만 sin으로 진동(슬롯 프레임 배경은 또렷하게 유지).
	if (ItemIconImage)
	{
		const float Time = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
		const float PulseT = (FMath::Sin(Time * PlaceholderPulseSpeed * 2.f * PI) + 1.f) * 0.5f;
		const float PulseAlpha = FMath::Lerp(PlaceholderPulseMinAlpha, PlaceholderPulseMaxAlpha, PulseT);
		ItemIconImage->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, PulseAlpha));
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
		// 옮길 대상(룻박스/창고/장착)이 있을 때만 빠른이동으로 클릭을 소비한다.
		// 대상이 없으면(인벤토리만 열림) 아래 드래그 감지로 넘어가, 달리기(Shift)로 Shift가 눌려 있어도 월드 드랍 드래그가 가능하다.
		if (TryHandleQuickTransfer())
		{
			return FReply::Handled();
		}
	}

	if (!CanStartItemDrag())
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
}

FReply ULSItemSlotWidget::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && TryHandleQuickTransfer())
	{
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDoubleClick(InGeometry, InMouseEvent);
}

FReply ULSItemSlotWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// Shift+좌클릭을 누른 채 커서를 슬롯 위로 쓸면 지나가는 칸을 차례로 빠른이동한다.
	// (장착된 칸은 ClearItem으로 비워져 bHasItem=false가 되므로 재호출돼도 무해)
	if (IsQuickTransferPointerEvent(InMouseEvent))
	{
		TryHandleQuickTransfer();
	}

	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

void ULSItemSlotWidget::NativeOnDragEnter(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragEnter(InGeometry, InDragDropEvent, InOperation);

	bIsDragTarget = IsValidInventoryDropTarget(InOperation) || IsValidLootDropTarget(InOperation) || IsValidWarehouseDropTarget(InOperation)
		|| IsValidChipEquipmentDropTarget(InOperation) || IsValidChipStationDropTarget(InOperation);
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
			// 드래그 비주얼은 아이템 아이콘만 커서를 따라가도록 배경 프레임을 숨긴다.
			DragVisual->bIsDragVisual = true;
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
	SetRenderScale(FVector2D::UnitVector);
	// 진행 중이던 등장 pop-in은 멈춘다. (다음 NativeTick이 다시 적용하므로 공개 직후 슬롯은 다시 popin 됨)
	bIsPopInAnimating = false;
}

void ULSItemSlotWidget::RefreshHoverStateFromCursor()
{
	// 리빌드 과정에서 ResetTransientSlotState가 bIsHovered를 끄지만, 커서가 그대로 이 슬롯 위에 있으면
	// 풀링 재사용된 위젯 인스턴스가 바뀌지 않아 Slate가 새 MouseEnter를 보내지 않는다.
	// 실제 Slate 호버 상태를 직접 조회해 호버 강조를 복원한다.
	const bool bActuallyHovered = IsHovered();
	if (bIsHovered == bActuallyHovered)
	{
		return;
	}

	bIsHovered = bActuallyHovered;
	ApplyHoverVisual();
}

void ULSItemSlotWidget::ApplyHoverVisual()
{
	// 미공개 placeholder는 호버/등급색을 적용하지 않는다(아이콘 펄스는 NativeTick이 담당).
	if (bIsPlaceholder)
	{
		return;
	}

	FLinearColor Tint = NormalIconTint;
	if (bIsLocked)
	{
		Tint = LockedIconTint;
	}
	else if (bIsDragTarget)
	{
		Tint = DragTargetIconTint;
	}
	else
	{
		Tint = bIsHovered ? HoveredIconTint : NormalIconTint;
	}

	// 배경은 특수 상태(잠금/드래그타겟/호버)에서는 피드백 틴트를, 평상시에는 아이템 등급색을 쓴다.
	// (빈 슬롯·등급 없는 아이템은 CurrentGradeBackgroundColor가 흰색이라 기존과 동일하게 보인다.)
	if (SlotBackgroundImage)
	{
		const bool bSpecialState = bIsLocked || bIsDragTarget || bIsHovered;
		SlotBackgroundImage->SetColorAndOpacity(bSpecialState ? Tint : CurrentGradeBackgroundColor);
	}

	if (ItemIconImage)
	{
		ItemIconImage->SetColorAndOpacity(Tint);
	}

	// 틴트에 더해 호버/드래그 타겟 시 슬롯을 살짝 키워 강조한다(잠금·드래그 비주얼 제외).
	const bool bShouldEmphasize = !bIsLocked && !bIsDragVisual && (bIsHovered || bIsDragTarget);
	SetRenderScale(bShouldEmphasize ? HoveredRenderScale : FVector2D::UnitVector);
}

void ULSItemSlotWidget::ApplySlotBackground()
{
	if (!SlotBackgroundImage)
	{
		UE_LOG(LogLS, Warning, TEXT("SlotBackgroundImage is not bound on %s."), *GetNameSafe(this));
		return;
	}

	// 드래그 비주얼은 아이템 아이콘만 표시하므로 배경 프레임을 숨긴다.
	if (bIsDragVisual)
	{
		SlotBackgroundImage->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	// DefaultSlotTexture가 지정된 경우에만 배경 브러시를 덮어쓴다.
	// 미지정이면 WBP 디자이너에서 설정한 배경 브러시를 그대로 둔다.
	if (DefaultSlotTexture)
	{
		SlotBackgroundImage->SetBrushFromTexture(DefaultSlotTexture);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("DefaultSlotTexture is not set on %s; using WBP designer brush for slot background."), *GetNameSafe(this));
	}

	SlotBackgroundImage->SetVisibility(ESlateVisibility::Visible);
}

FLinearColor ULSItemSlotWidget::ResolveGradeBackgroundColor(const FName ItemRowName) const
{
	// 등급 토큰은 툴팁(GetGradeText)과 동일한 단일 출처에서 파싱한다.
	const FString Grade = LSInventorySlotUtils::ResolveItemGradeFromRowName(ItemRowName);
	if (Grade == TEXT("Supply"))       return SupplyGradeColor;
	if (Grade == TEXT("Standard"))     return StandardGradeColor;
	if (Grade == TEXT("Precision"))    return PrecisionGradeColor;
	if (Grade == TEXT("Tuning"))       return TuningGradeColor;
	if (Grade == TEXT("Prototype"))    return PrototypeGradeColor;
	if (Grade == TEXT("Masterpiece"))  return MasterpieceGradeColor;

	return DefaultGradeColor;
}

bool ULSItemSlotWidget::CanStartItemDrag() const
{
	if (bIsPlaceholder)
	{
		return false;
	}

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
	// 미공개 placeholder는 빠른 이동(Shift+클릭) 불가.
	if (bIsPlaceholder)
	{
		return false;
	}

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
	if (!OwningChipStation || !OwningChipStation->QuickEquipChipToFirstEmptyHardwareSlot(SlotArea, SlotIndex))
	{
		return false;
	}

	// 칩 리스트는 재정렬/리빌드하지 않고 이 소스 슬롯 한 칸만 비운다(정렬은 스테이션을 다시 열 때만).
	ClearItem();
	return true;
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

bool ULSItemSlotWidget::IsValidChipEquipmentDropTarget(const UDragDropOperation* InOperation) const
{
	// 칩 장착 슬롯이 드롭 대상일 때의 유효 조건. HandleChipDrop의 수락 조건과 일치한다.
	const ULSInventoryDragDropOperation* DragOperation = Cast<ULSInventoryDragDropOperation>(InOperation);
	if (!DragOperation || bIsLocked || !ChipEquipmentSlotWidget.IsValid())
	{
		return false;
	}

	if (!DragOperation->SourceChipStationWidget || DragOperation->DragItemRowName.IsNone() || DragOperation->DragAmount <= 0)
	{
		return false;
	}

	if (ChipStationWidget.IsValid() && DragOperation->SourceChipStationWidget != ChipStationWidget.Get())
	{
		return false;
	}

	return DragOperation->DragItemRowName.ToString().StartsWith(TEXT("Chip_"));
}

bool ULSItemSlotWidget::IsValidChipStationDropTarget(const UDragDropOperation* InOperation) const
{
	// 칩 목록 슬롯은 장착 슬롯에서 드래그해 온 경우(장착 해제 스왑)에만 유효 대상이다. NativeOnDrop의 칩 스테이션 분기와 일치한다.
	const ULSInventoryDragDropOperation* DragOperation = Cast<ULSInventoryDragDropOperation>(InOperation);
	if (!DragOperation || bIsLocked || ChipEquipmentSlotWidget.IsValid() || !ChipStationWidget.IsValid())
	{
		return false;
	}

	return DragOperation->SourceChipEquipmentSlotWidget != nullptr;
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
