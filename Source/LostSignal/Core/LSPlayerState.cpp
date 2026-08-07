#include "Core/LSPlayerState.h"

#include "Inventory/LSInventorySlotUtils.h"
#include "LostSignal.h"

void ALSPlayerState::CopyProperties(APlayerState* NewPlayerState)
{
	Super::CopyProperties(NewPlayerState);

	// seamless travel에서 PlayerState는 새로 만들어지고 값만 여기로 복사된다.
	// (APlayerController::SeamlessTravelFrom -> APlayerState::SeamlessTravelTo -> CopyProperties)
	// 이 복사를 빼먹으면 파밍 레벨에서 빈 payload로 레이드가 시작된다.
	ALSPlayerState* NewLSPlayerState = Cast<ALSPlayerState>(NewPlayerState);
	if (!NewLSPlayerState)
	{
		UE_LOG(LogLS, Warning, TEXT("[PlayerState] Raid entry data was dropped during travel because %s is not an ALSPlayerState."),
			*GetNameSafe(NewPlayerState));
		return;
	}

	NewLSPlayerState->bHasRaidEntryData = bHasRaidEntryData;
	NewLSPlayerState->RaidLoadout = RaidLoadout;
	NewLSPlayerState->RaidSafeItems = RaidSafeItems;
	NewLSPlayerState->RaidEquipment = RaidEquipment;
}

void ALSPlayerState::StoreRaidEntryData(const TArray<FLSSessionItem>& Loadout, const TArray<FLSSessionItem>& SafeItems, const TArray<FLSSessionItem>& EquipmentItems)
{
	RaidLoadout = Loadout;
	RaidSafeItems = SafeItems;
	LSInventorySlotUtils::NormalizeSlotArray(RaidLoadout);
	LSInventorySlotUtils::NormalizeSlotArray(RaidSafeItems);
	// 장비는 인덱스=슬롯 타입이므로 Normalize(빈 칸 압축) 금지 — 5칸 패딩만 한다.
	RaidEquipment = EquipmentItems;
	RaidEquipment.SetNum(LSInventorySlotUtils::EquipmentSlotCount);
	bHasRaidEntryData = true;
}

void ALSPlayerState::ClearRaidEntryData()
{
	bHasRaidEntryData = false;
	RaidLoadout.Reset();
	RaidSafeItems.Reset();
	RaidEquipment.Reset();
}
