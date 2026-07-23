#include "UI/QuickSlot/LSQuickSlotWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Core/LSPlayerControllerBase.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Inventory/LSCraftingUtils.h"
#include "Inventory/LSInventorySlotUtils.h"
#include "Inventory/LSRaidInventoryComponent.h"
#include "LostSignal.h"
#include "Session/LSSaveSubsystem.h"
#include "UI/Inventory/LSInventoryDragDropOperation.h"
#include "UI/LootDrop/LSLootDropWidget.h"

void ULSQuickSlotWidget::InitializeSlot(const int32 InSlotIndex)
{
	SlotIndex = InSlotIndex;
	// WBP가 지정한 배경 색을 캡처한다(호버 해제 시 이 색으로 복원). Refresh 전에 1회만.
	if (SlotBackgroundImage)
	{
		NormalBackgroundColor = SlotBackgroundImage->GetColorAndOpacity();
	}
	Refresh();
}

void ULSQuickSlotWidget::Refresh()
{
	if (!IconImage || !AmountText)
	{
		UE_LOG(LogLS, Warning, TEXT("[QuickSlot] IconImage/AmountText not bound on %s."), *GetNameSafe(this));
		return;
	}

	const ULSSaveSubsystem* SaveSubsystem = ResolveSaveSubsystem();
	FName ItemRowName = NAME_None;
	if (SaveSubsystem && SaveSubsystem->GetQuickSlots().IsValidIndex(SlotIndex))
	{
		ItemRowName = SaveSubsystem->GetQuickSlots()[SlotIndex];
	}

	// 빈 칸: 아이콘/개수 숨김.
	if (ItemRowName.IsNone())
	{
		IconImage->SetVisibility(ESlateVisibility::Collapsed);
		AmountText->SetText(FText::GetEmpty());
		ApplyHoverVisual();
		return;
	}

	if (UTexture2D* IconTexture = LSInventorySlotUtils::LoadItemIconTexture(ItemRowName))
	{
		IconImage->SetBrushFromTexture(IconTexture);
	}
	IconImage->SetVisibility(ESlateVisibility::HitTestInvisible);

	AmountText->SetText(FText::AsNumber(CountOwnedAmount(ItemRowName)));

	// 리드로 후에도 현재 호버 상태의 강조를 유지한다.
	ApplyHoverVisual();
}

FReply ULSQuickSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 우클릭으로 등록 해제.
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		if (ULSSaveSubsystem* SaveSubsystem = ResolveSaveSubsystem())
		{
			SaveSubsystem->ClearQuickSlot(SlotIndex);
		}
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

bool ULSQuickSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	const ULSInventoryDragDropOperation* DragOp = Cast<ULSInventoryDragDropOperation>(InOperation);
	if (!DragOp)
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	// 루팅 박스에서 온 드롭이면 먼저 인벤토리로 루팅한다. 퀵슬롯은 참조만 저장하므로
	// 루팅하지 않으면 인벤토리에 없는 아이템을 가리켜 개수 0으로 표시된다.
	// 루팅 실패(인벤토리 가득 등)면 등록하지 않고 아이템은 박스에 유지한다.
	if (ULSLootDropWidget* LootSource = DragOp->SourceLootDropWidget)
	{
		if (!LootSource->TransferLootSlotToInventory(DragOp->SourceSlotIndex))
		{
			UE_LOG(LogLS, Warning, TEXT("[QuickSlot] 루팅 실패로 등록 취소: '%s' (인벤토리 가득?)."), *DragOp->DragItemRowName.ToString());
			return true;
		}
	}

	// 소모품 여부/등록 성공 여부와 무관하게 드롭을 소비(true)한다. 이 위젯은 인벤토리 슬롯을 옮기지 않으므로
	// 인벤토리 원본은 그대로 남고, SetQuickSlot이 소모품이 아니면 내부에서 거부 로그를 남긴다.
	if (ULSSaveSubsystem* SaveSubsystem = ResolveSaveSubsystem())
	{
		SaveSubsystem->SetQuickSlot(SlotIndex, DragOp->DragItemRowName);
	}

	return true;
}

void ULSQuickSlotWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	// 인벤토리 패널이 열려 있을 때만 호버 강조한다(HUD에 상시 떠 있는 바는 전투 중 무반응).
	bIsHovered = IsInventoryContextOpen();
	ApplyHoverVisual();
}

void ULSQuickSlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	bIsHovered = false;
	ApplyHoverVisual();
}

void ULSQuickSlotWidget::ApplyHoverVisual()
{
	SetRenderScale(bIsHovered ? HoveredRenderScale : FVector2D::UnitVector);
	if (IconImage)
	{
		IconImage->SetColorAndOpacity(bIsHovered ? HoveredIconTint : FLinearColor::White);
	}
	if (SlotBackgroundImage)
	{
		SlotBackgroundImage->SetColorAndOpacity(bIsHovered ? HoveredIconTint : NormalBackgroundColor);
	}
}

bool ULSQuickSlotWidget::IsInventoryContextOpen() const
{
	const ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer());
	return PlayerController && PlayerController->IsInventoryUIOpen();
}

ULSSaveSubsystem* ULSQuickSlotWidget::ResolveSaveSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
}

int32 ULSQuickSlotWidget::CountOwnedAmount(const FName ItemRowName) const
{
	if (ItemRowName.IsNone())
	{
		return 0;
	}

	// 레이드 중이면 서버 미러(세션) 인벤토리를, 아니면 로비 세이브 인벤토리를 합산한다.
	// 툴팁 "현재 아이템 개수"와 동일하게 운반 중 소지량(일반 + Safe)을 기준으로 한다.
	if (const ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer()))
	{
		if (const ULSRaidInventoryComponent* RaidInventory = PlayerController->GetRaidInventoryComponent();
			RaidInventory && RaidInventory->IsRaidActive())
		{
			return LSCraftingUtils::CountItem(RaidInventory->GetSessionInventory(), ItemRowName)
				+ LSCraftingUtils::CountItem(RaidInventory->GetSessionSafeInventory(), ItemRowName);
		}
	}

	if (const ULSSaveSubsystem* SaveSubsystem = ResolveSaveSubsystem())
	{
		return LSCraftingUtils::CountItem(SaveSubsystem->GetInventory(), ItemRowName)
			+ LSCraftingUtils::CountItem(SaveSubsystem->GetSafeStash(), ItemRowName);
	}

	return 0;
}
