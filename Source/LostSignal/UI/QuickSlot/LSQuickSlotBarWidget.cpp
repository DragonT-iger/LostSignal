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

	// 등록 변경(다른 경로의 SetQuickSlot/ClearQuickSlot 포함)이 발생하면 아이콘을 다시 그린다.
	if (ULSSaveSubsystem* SaveSubsystem = ResolveSaveSubsystem())
	{
		QuickSlotsChangedHandle = SaveSubsystem->OnQuickSlotsChanged.AddUObject(this, &ULSQuickSlotBarWidget::RefreshAll);
	}

	// 인벤토리 변경 funnel(RefreshAllInventoryUI)이 개수 갱신을 태울 수 있도록 활성 바로 등록한다.
	if (ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(GetOwningPlayer()))
	{
		PlayerController->RegisterQuickSlotBar(this);
	}
}

void ULSQuickSlotBarWidget::NativeDestruct()
{
	if (QuickSlotsChangedHandle.IsValid())
	{
		if (ULSSaveSubsystem* SaveSubsystem = ResolveSaveSubsystem())
		{
			SaveSubsystem->OnQuickSlotsChanged.Remove(QuickSlotsChangedHandle);
		}
		QuickSlotsChangedHandle.Reset();
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
}

ULSSaveSubsystem* ULSQuickSlotBarWidget::ResolveSaveSubsystem() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
}
