#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "Gameplay/LSInteractableObject.h"
#include "Data/LSDropSubsystem.h"
#include "Session/LSSessionSubsystem.h"
#include "LSLootBox.generated.h"

class ULSRaidInventoryComponent;
class ULSSaveSubsystem;
class ULSMinimapMarkerComponent;

UCLASS()
class LOSTSIGNAL_API ALSLootBox : public ALSInteractableObject
{
	GENERATED_BODY()

public:
	ALSLootBox();

	virtual bool CanInteract_Implementation(APawn* Interactor) override;
	virtual void Interact_Implementation(APawn* Interactor) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	const TArray<FLSDropResult>& GetLootResults() const { return LootResults; }
	// 클라가 미공개 placeholder 슬롯을 그릴 수 있도록 총 드랍 개수만 노출한다(아이템 정체는 비복제).
	int32 GetTotalLootCount() const { return TotalLootCount; }
	bool DropLootSlot(int32 FromLootSlotIndex, int32 ToLootSlotIndex);
	bool TransferLootSlotToSession(int32 LootSlotIndex, ULSRaidInventoryComponent* RaidInventory, FLSSessionItem& OutRemainingLootItem);
	bool TransferLootSlotToSessionSlot(int32 LootSlotIndex, ULSRaidInventoryComponent* RaidInventory, ELSInventorySlotArea ToSlotArea, int32 ToSlotIndex, FLSSessionItem& OutRemainingLootItem);
	bool TransferSessionSlotToLootSlot(int32 LootSlotIndex, ULSRaidInventoryComponent* RaidInventory, ELSInventorySlotArea FromSlotArea, int32 FromSlotIndex, FLSSessionItem& OutLootItem);

	// 로비 파밍용: 레이드 세션이 없을 때 룻박스 아이템을 영구 세이브(SaveSubsystem)에 직접 전송한다.
	bool TransferLootSlotToSave(int32 LootSlotIndex, ULSSaveSubsystem* SaveSubsystem, FLSSessionItem& OutRemainingLootItem);
	bool TransferLootSlotToSaveSlot(int32 LootSlotIndex, ULSSaveSubsystem* SaveSubsystem, ELSInventorySlotArea ToSlotArea, int32 ToSlotIndex, FLSSessionItem& OutRemainingLootItem);
	bool TransferSaveSlotToLootSlot(int32 LootSlotIndex, ULSSaveSubsystem* SaveSubsystem, ELSInventorySlotArea FromSlotArea, int32 FromSlotIndex, FLSSessionItem& OutLootItem);

	UFUNCTION(BlueprintImplementableEvent, Category="LS/Loot")
	void OnLootResultReceived(const TArray<FLSDropResult>& Results);

	// 박스가 열릴 때(서버/모든 클라 공통) 메시 오픈·발광·오픈 SFX. 실제 연출은 BP에서 구현.
	UFUNCTION(BlueprintImplementableEvent, Category="LS/Loot")
	void OnLootBoxOpenedVisual();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LS/Loot")
	FName RootingObjectRowName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Minimap")
	TObjectPtr<ULSMinimapMarkerComponent> MinimapMarkerComponent;

private:
	UPROPERTY(ReplicatedUsing=OnRep_IsOpened, VisibleInstanceOnly, Category="LS/Loot")
	bool bIsOpened = false;

	UPROPERTY(ReplicatedUsing=OnRep_LootResults, Transient, VisibleInstanceOnly, Category="LS/Loot")
	TArray<FLSDropResult> LootResults;

	// 서버 전용 전체 드랍 결과(복제 안 함). 미공개 아이템 데이터가 클라로 새지 않게 하는 핵심.
	// 타이머가 여기서 하나씩 꺼내 복제되는 LootResults에 append하며 단계 공개한다.
	UPROPERTY(Transient)
	TArray<FLSDropResult> PendingLootResults;

	// 총 드랍 개수(미공개 placeholder 표시용). 아이템 정체가 아닌 개수만 복제한다.
	UPROPERTY(ReplicatedUsing=OnRep_LootResults, Transient, VisibleInstanceOnly, Category="LS/Loot")
	int32 TotalLootCount = 0;

	// 다음에 공개할 PendingLootResults 인덱스 (서버 전용).
	int32 NextRevealIndex = 0;

	// 다음 아이템 공개 예약 타이머 (서버 전용).
	FTimerHandle RevealTimerHandle;

	UFUNCTION()
	void OnRep_IsOpened();

	UFUNCTION()
	void OnRep_LootResults();

	void ClearLootSlot(int32 LootSlotIndex);
	void NotifyLootResultsChanged();

	// 단계 공개: 다음 아이템을 등급 딜레이 후 공개하도록 예약한다. 남은 게 없으면 타이머를 정리한다.
	void ScheduleNextReveal();
	// 타이머 콜백: PendingLootResults에서 한 개를 LootResults로 옮기고 다음 공개를 예약한다.
	void RevealNextLootItem();
	// 아이템 등급(RowName 파싱)으로 공개 전 대기 시간을 LSDropSettings에서 조회한다.
	float GetItemRevealDelay(const FLSDropResult& Item) const;
};
