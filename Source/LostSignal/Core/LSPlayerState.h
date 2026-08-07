#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Session/LSSessionSubsystem.h"
#include "LSPlayerState.generated.h"

// 레벨 전환을 건너뛰어야 하는 플레이어별 서버 상태를 보관한다.
//
// seamless travel은 PlayerController를 호스트 것까지 전부 새로 스폰하므로
// (AGameModeBase::HandleSeamlessTravelPlayer) 입장 payload를 컨트롤러에 두면 파밍 레벨에서 전원 유실된다.
// PlayerState도 같은 객체가 넘어오는 것이 아니라 새로 만들어지고 CopyProperties로 값만 복사되므로,
// 아래 CopyProperties 오버라이드가 없으면 조용히 빈 payload로 레이드가 시작된다.
UCLASS()
class LOSTSIGNAL_API ALSPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void CopyProperties(APlayerState* NewPlayerState) override;

	// 입장 payload 저장. 인벤/금고는 정규화하고, 장비는 인덱스=슬롯 타입이라 5칸 패딩만 한다.
	void StoreRaidEntryData(const TArray<FLSSessionItem>& Loadout, const TArray<FLSSessionItem>& SafeItems, const TArray<FLSSessionItem>& EquipmentItems);
	void ClearRaidEntryData();

	bool HasRaidEntryData() const { return bHasRaidEntryData; }
	const TArray<FLSSessionItem>& GetRaidLoadout() const { return RaidLoadout; }
	const TArray<FLSSessionItem>& GetRaidSafeItems() const { return RaidSafeItems; }
	const TArray<FLSSessionItem>& GetRaidEquipment() const { return RaidEquipment; }

private:
	// 서버 전용이라 리플리케이트하지 않는다. 클라이언트는 RaidInventoryComponent 미러로 본다
	// (ClientStartRaidSession / ClientSyncRaidSessionAndLoot).
	UPROPERTY(Transient)
	bool bHasRaidEntryData = false;

	UPROPERTY(Transient)
	TArray<FLSSessionItem> RaidLoadout;

	UPROPERTY(Transient)
	TArray<FLSSessionItem> RaidSafeItems;

	// 무기/방어구 장착 5칸. 인덱스=슬롯 타입이므로 정렬/압축 금지.
	UPROPERTY(Transient)
	TArray<FLSSessionItem> RaidEquipment;
};
