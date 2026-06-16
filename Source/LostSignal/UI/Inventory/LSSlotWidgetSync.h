#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"

class APlayerController;
class ULSItemSlotWidget;
class UWrapBox;
class UWorld;

namespace LSSlotWidgetSync
{
	// WrapBox의 슬롯 자식 수를 DesiredCount에 맞춘다.
	// 기존 자식은 유지해 재사용하고, 부족한 슬롯만 생성하며, 초과 슬롯은 뒤에서 제거한다.
	// 각 슬롯은 임시 상호작용 상태를 초기화한 뒤 ConfigureSlot으로 내용만 갱신한다.
	void SyncSlotWidgets(
		UWrapBox* WrapBox,
		const TSubclassOf<ULSItemSlotWidget>& SlotWidgetClass,
		APlayerController* OwningPlayer,
		UWorld* World,
		int32 DesiredCount,
		TFunctionRef<void(int32 SlotIndex, ULSItemSlotWidget& SlotWidget)> ConfigureSlot);
}
