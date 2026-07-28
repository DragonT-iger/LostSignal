#include "UI/QuickSlot/LSQuickSlotBarWidget.h"

#include "Core/LSPlayerControllerBase.h"
#include "Engine/GameInstance.h"
#include "LostSignal.h"
#include "Session/LSSaveSubsystem.h"
#include "UI/QuickSlot/LSQuickSlotWidget.h"

void ULSQuickSlotBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	InitializeBar();

	UE_LOG(LogLS, Log, TEXT("[QuickSlot] Bar %s constructed (bHideLockedSlots=%s, Outer=%s)."),
		*GetNameSafe(this), bHideLockedSlots ? TEXT("true") : TEXT("false"), *GetNameSafe(GetOuter()));

	// 등록 변경(다른 경로의 SetQuickSlot/ClearQuickSlot 포함)이 발생하면 아이콘을 다시 그린다.
	if (ULSSaveSubsystem* SaveSubsystem = ResolveSaveSubsystem())
	{
		QuickSlotsChangedHandle = SaveSubsystem->OnQuickSlotsChanged.AddUObject(this, &ULSQuickSlotBarWidget::RefreshAll);
		// 칩 장착/신호 게이지 변경은 적재 프로토콜 레벨을 바꾸므로 해금 칸 가시성을 다시 평가한다.
		ChipLoadoutChangedHandle = SaveSubsystem->OnChipLoadoutChanged.AddUObject(this, &ULSQuickSlotBarWidget::RefreshAll);
	}

	// 인벤토리 변경 funnel(RefreshAllInventoryUI)이 개수 갱신을 태울 수 있도록 활성 바로 등록한다.
	if (ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer()))
	{
		PlayerController->RegisterQuickSlotBar(this);
	}
}

void ULSQuickSlotBarWidget::NativeDestruct()
{
	if (QuickSlotsChangedHandle.IsValid() || ChipLoadoutChangedHandle.IsValid())
	{
		if (ULSSaveSubsystem* SaveSubsystem = ResolveSaveSubsystem())
		{
			SaveSubsystem->OnQuickSlotsChanged.Remove(QuickSlotsChangedHandle);
			SaveSubsystem->OnChipLoadoutChanged.Remove(ChipLoadoutChangedHandle);
		}
		QuickSlotsChangedHandle.Reset();
		ChipLoadoutChangedHandle.Reset();
	}

	if (ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer()))
	{
		PlayerController->UnregisterQuickSlotBar(this);
	}

	Super::NativeDestruct();
}

void ULSQuickSlotBarWidget::InitializeBar()
{
	Slots = { QuickSlot1, QuickSlot2, QuickSlot3, QuickSlot4, QuickSlot5, QuickSlot6 };

	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		if (!Slots[Index])
		{
			UE_LOG(LogLS, Warning, TEXT("[QuickSlot] QuickSlot%d is not bound on %s."), Index + 1, *GetNameSafe(this));
			continue;
		}
		Slots[Index]->InitializeSlot(Index);
	}

	ApplyProtocolVisibility();
}

void ULSQuickSlotBarWidget::RefreshAll()
{
	for (ULSQuickSlotWidget* SlotWidget : Slots)
	{
		if (SlotWidget)
		{
			SlotWidget->Refresh();
		}
	}

	ApplyProtocolVisibility();
}

void ULSQuickSlotBarWidget::ApplyProtocolVisibility()
{
	// 인벤토리 바 등 접지 않는 바는 6칸을 항상 표시한다(이전에 접혔던 상태도 복원).
	if (!bHideLockedSlots)
	{
		for (ULSQuickSlotWidget* SlotWidget : Slots)
		{
			if (SlotWidget)
			{
				SlotWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			}
		}
		return;
	}

	// HUD 바: 적재 프로토콜로 해금된 칸만 표시하고 나머지는 접는다(표시 영역 리플로우).
	const ULSSaveSubsystem* SaveSubsystem = ResolveSaveSubsystem();
	const int32 UnlockedCount = SaveSubsystem ? SaveSubsystem->GetUnlockedQuickSlotCount() : 0;
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		if (Slots[Index])
		{
			Slots[Index]->SetVisibility(Index < UnlockedCount ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		}
	}
}

ULSSaveSubsystem* ULSQuickSlotBarWidget::ResolveSaveSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
}
