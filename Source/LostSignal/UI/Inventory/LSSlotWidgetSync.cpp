#include "UI/Inventory/LSSlotWidgetSync.h"

#include "Blueprint/UserWidget.h"
#include "Components/WrapBox.h"
#include "LostSignal.h"
#include "UI/Inventory/LSItemSlotWidget.h"

namespace LSSlotWidgetSync
{
namespace
{
ULSItemSlotWidget* CreateSlotWidget(const TSubclassOf<ULSItemSlotWidget>& SlotWidgetClass, APlayerController* OwningPlayer, UWorld* World)
{
	if (OwningPlayer)
	{
		return CreateWidget<ULSItemSlotWidget>(OwningPlayer, SlotWidgetClass);
	}

	return CreateWidget<ULSItemSlotWidget>(World, SlotWidgetClass);
}
}

void SyncSlotWidgets(
	UWrapBox* WrapBox,
	const TSubclassOf<ULSItemSlotWidget>& SlotWidgetClass,
	APlayerController* OwningPlayer,
	UWorld* World,
	int32 DesiredCount,
	TFunctionRef<void(int32 SlotIndex, ULSItemSlotWidget& SlotWidget)> ConfigureSlot)
{
	if (!WrapBox)
	{
		UE_LOG(LogLS, Warning, TEXT("SyncSlotWidgets aborted because WrapBox is null."));
		return;
	}

	DesiredCount = FMath::Max(0, DesiredCount);

	// 초과분은 뒤에서부터 제거해 기존 슬롯 순서를 유지한다.
	for (int32 ChildIndex = WrapBox->GetChildrenCount() - 1; ChildIndex >= DesiredCount; --ChildIndex)
	{
		WrapBox->RemoveChildAt(ChildIndex);
	}

	for (int32 SlotIndex = 0; SlotIndex < DesiredCount; ++SlotIndex)
	{
		ULSItemSlotWidget* SlotWidget = nullptr;
		if (SlotIndex < WrapBox->GetChildrenCount())
		{
			SlotWidget = Cast<ULSItemSlotWidget>(WrapBox->GetChildAt(SlotIndex));
			if (!SlotWidget)
			{
				// 예상하지 못한 타입의 자식이면 해당 지점부터 비우고 새 슬롯으로 채운다.
				for (int32 ChildIndex = WrapBox->GetChildrenCount() - 1; ChildIndex >= SlotIndex; --ChildIndex)
				{
					WrapBox->RemoveChildAt(ChildIndex);
				}
			}
		}

		if (!SlotWidget)
		{
			if (!SlotWidgetClass)
			{
				UE_LOG(LogLS, Warning, TEXT("SyncSlotWidgets aborted because SlotWidgetClass is null."));
				return;
			}

			if (!OwningPlayer && !World)
			{
				UE_LOG(LogLS, Warning, TEXT("SyncSlotWidgets aborted because owner/world is missing."));
				return;
			}

			SlotWidget = CreateSlotWidget(SlotWidgetClass, OwningPlayer, World);
			if (!SlotWidget)
			{
				UE_LOG(LogLS, Warning, TEXT("Failed to create slot widget at index %d during SyncSlotWidgets."), SlotIndex);
				return;
			}

			// 이 시점의 자식 수는 SlotIndex와 같으므로 끝에 추가하면 올바른 위치가 된다.
			WrapBox->AddChildToWrapBox(SlotWidget);
		}

		SlotWidget->ResetTransientSlotState();
		ConfigureSlot(SlotIndex, *SlotWidget);
	}
}
}
