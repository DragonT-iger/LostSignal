#include "UI/QuickSlot/LSQuickSlotWidget.h"

#include "Characters/LSPlayerCharacter.h"
#include "Components/TextBlock.h"
#include "Core/LSPlayerControllerBase.h"
#include "EnhancedActionKeyMapping.h"
#include "Engine/GameInstance.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "Inventory/LSCraftingUtils.h"
#include "Inventory/LSRaidInventoryComponent.h"
#include "LostSignal.h"
#include "Session/LSSaveSubsystem.h"
#include "UI/Inventory/LSInventoryDragDropOperation.h"
#include "UI/Inventory/LSItemSlotWidget.h"
#include "UI/LootDrop/LSLootDropWidget.h"

void ULSQuickSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ItemSlot)
	{
		ItemSlot->SetDisplayOnlySlotContext();
		ItemSlot->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("[QuickSlot] ItemSlot not bound on %s."), *GetNameSafe(this));
	}

	if (!BindKeyText)
	{
		UE_LOG(LogLS, Warning, TEXT("[QuickSlot] BindKeyText not bound on %s."), *GetNameSafe(this));
	}
	RefreshShortcutText();
}

void ULSQuickSlotWidget::InitializeSlot(const int32 InSlotIndex)
{
	SlotIndex = InSlotIndex;
	RefreshShortcutText();
	Refresh();
}

void ULSQuickSlotWidget::Refresh()
{
	if (!ItemSlot)
	{
		UE_LOG(LogLS, Warning, TEXT("[QuickSlot] ItemSlot not bound on %s."), *GetNameSafe(this));
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
		ItemSlot->ClearItem();
		ApplyHoverVisual();
		return;
	}

	const int32 OwnedAmount = CountOwnedAmount(ItemRowName);
	ItemSlot->SetItem(ItemRowName, OwnedAmount, {});
	// 퀵슬롯은 등록 참조를 유지하므로 미보유 상태도 숫자 0을 명시한다.
	ItemSlot->SetDisplayedAmount(OwnedAmount);

	// 리드로 후에도 현재 호버 상태의 강조를 유지한다.
	ApplyHoverVisual();
}

FReply ULSQuickSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 우클릭으로 등록 해제. 인벤토리 패널이 열려 있을 때만 허용한다(HUD 상시 바는 전투 중 해제 방지).
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton && IsInventoryContextOpen())
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
	if (ItemSlot)
	{
		ItemSlot->SetExternalHoverState(bIsHovered);
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

void ULSQuickSlotWidget::RefreshShortcutText()
{
	if (!BindKeyText)
	{
		return;
	}

	BindKeyText->SetText(ResolveShortcutText());
	BindKeyText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

FText ULSQuickSlotWidget::ResolveShortcutText() const
{
	// 소비는 레이드 폰(ALSPlayerCharacter) 전용이라 매핑도 폰에서 조회한다.
	// 로비 인벤토리엔 폰이 없어 매핑을 못 찾으므로 빈 텍스트로 둔다(잘못된 키를 표시하지 않는다).
	const APlayerController* PlayerController = GetOwningPlayer();
	const ALSPlayerCharacter* PlayerCharacter = PlayerController ? Cast<ALSPlayerCharacter>(PlayerController->GetPawn()) : nullptr;
	const UInputAction* InputAction = PlayerCharacter ? PlayerCharacter->GetItemInputAction(SlotIndex) : nullptr;
	return ResolveShortcutTextFromInputMappings(InputAction);
}

FText ULSQuickSlotWidget::ResolveShortcutTextFromInputMappings(const UInputAction* InputAction) const
{
	const ALSPlayerControllerBase* PlayerController = GetOwningPlayer<ALSPlayerControllerBase>();
	if (!PlayerController || !InputAction)
	{
		return FText::GetEmpty();
	}

	FKey FirstValidKey;
	for (const UInputMappingContext* MappingContext : PlayerController->GetDefaultMappingContexts())
	{
		if (!MappingContext)
		{
			continue;
		}

		for (const FEnhancedActionKeyMapping& Mapping : MappingContext->GetMappings())
		{
			if (Mapping.Action != InputAction || !Mapping.Key.IsValid())
			{
				continue;
			}

			if (!FirstValidKey.IsValid())
			{
				FirstValidKey = Mapping.Key;
			}

			if (!Mapping.Key.IsGamepadKey())
			{
				return Mapping.Key.GetDisplayName(false);
			}
		}
	}

	return FirstValidKey.IsValid() ? FirstValidKey.GetDisplayName(false) : FText::GetEmpty();
}
