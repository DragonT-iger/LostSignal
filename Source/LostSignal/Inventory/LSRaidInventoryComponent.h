#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Session/LSSessionSubsystem.h"
#include "LSRaidInventoryComponent.generated.h"

UCLASS(ClassGroup=(LS), meta=(BlueprintSpawnableComponent))
class LOSTSIGNAL_API ULSRaidInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSRaidInventoryComponent();

	void StartRaidInventory(const TArray<FLSSessionItem>& Loadout, const TArray<FLSSessionItem>& SafeItems, const TArray<FLSSessionItem>& EquipmentItems);
	void MirrorRaidInventoryState(const TArray<FLSSessionItem>& InventoryItems, const TArray<FLSSessionItem>& SafeItems, const TArray<FLSSessionItem>& EquipmentItems);
	void EndRaidInventory();

	bool IsRaidActive() const { return bRaidActive; }
	int32 GetMaxInventorySlotCount() const;
	int32 GetMaxSafeSlotCount() const;
	bool IsSessionSlotAccessible(ELSInventorySlotArea SlotArea, int32 SlotIndex) const;

	const TArray<FLSSessionItem>& GetSessionInventory() const { return SessionInventory; }
	const TArray<FLSSessionItem>& GetSessionSafeInventory() const { return SessionSafeInventory; }
	const TArray<FLSSessionItem>& GetSessionEquipmentSlots() const { return SessionEquipmentSlots; }

	bool TryAddSessionItem(FName ItemRowName, int32 Amount, const TArray<FLSChipResolvedStat>& ChipStats, FLSSessionItem& OutRemainingItem);
	void SortSessionInventory();
	bool DropSessionSlot(ELSInventorySlotArea FromArea, int32 FromIndex, ELSInventorySlotArea ToArea, int32 ToIndex);
	bool DropExternalItemToSessionSlot(FLSSessionItem& InOutExternalItem, ELSInventorySlotArea ToArea, int32 ToIndex);
	bool GetSessionSlotItem(ELSInventorySlotArea SlotArea, int32 SlotIndex, FLSSessionItem& OutItem) const;
	bool ClearSessionSlot(ELSInventorySlotArea SlotArea, int32 SlotIndex);
	// 서버의 적재 용량 초과분 정리 전용. 현재 최대치 뒤의 아이템을 모두 꺼내고 원본 슬롯을 비운다.
	int32 ExtractOverflowInventoryItems(TArray<FLSSessionItem>& OutItems);
	// 세션 인벤토리(금고 제외)에서 ItemRowName을 Amount만큼 제거한다. 서버 권한 가정. 실제 제거된 수량 반환.
	// 소모품 사용 시 수량 차감에 쓴다(차감 소스는 일반 인벤토리로 한정).
	int32 ConsumeSessionItem(FName ItemRowName, int32 Amount);
	bool ReplaceSessionSlotItem(ELSInventorySlotArea SlotArea, int32 SlotIndex, const FLSSessionItem& NewItem, FLSSessionItem& OutPreviousItem);

private:
	// 슬롯 영역에 대응하는 세션 배열을 돌려준다. Warehouse처럼 레이드가 다루지 않는 영역이면 nullptr.
	TArray<FLSSessionItem>* ResolveSessionSlots(ELSInventorySlotArea SlotArea);
	const TArray<FLSSessionItem>* ResolveSessionSlots(ELSInventorySlotArea SlotArea) const;
	int32 ResolveMaxSlotCount(ELSInventorySlotArea SlotArea) const;

	bool bRaidActive = false;
	TArray<FLSSessionItem> SessionInventory;
	TArray<FLSSessionItem> SessionSafeInventory;
	// 무기/방어구 장착 5칸. 인덱스=슬롯 타입(ELSEquipmentSlot)이므로 정렬/압축 금지, 항상 5칸 패딩 유지.
	TArray<FLSSessionItem> SessionEquipmentSlots;
	TArray<FLSSessionItem> ConsumedItems;
	FLSLoadoutSnapshot LoadoutSnapshot;
};
