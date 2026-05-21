#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LSSessionSubsystem.generated.h"

UENUM(BlueprintType)
enum class ELSRaidResult : uint8
{
	Extracted,	// 탈출 성공 - 획득 아이템 전부 보관
	Quit,		// 중도 탈주 - bAllowQuitRecovery에 따라 출발 장비 복구
	Dead,		// 사망 - 전부 소실
};

// 레이드 중 아이템 단위 (파밍 획득 / 소모 / 출발 장비 공용)
UENUM(BlueprintType)
enum class ELSInventorySlotArea : uint8
{
	Inventory,
	Safe,
	Warehouse,
};

USTRUCT(BlueprintType)
struct FLSSessionItem
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FName ItemRowName;
	UPROPERTY(BlueprintReadOnly) int32 Amount = 0;
};

// 레이드 입장 시점의 장비 스냅샷
USTRUCT(BlueprintType)
struct FLSLoadoutSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) TArray<FLSSessionItem> Items;
};

UCLASS()
class LOSTSIGNAL_API ULSSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// 탈주 시 출발 장비 복구 허용 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LS/Session")
	bool bAllowQuitRecovery = false;

	// 레이드 시작 - 스냅샷 찍고 세션 초기화
	UFUNCTION(BlueprintCallable, Category="LS/Session")
	void StartRaid(const TArray<FLSSessionItem>& Loadout);

	void StartRaidClientMirror(const TArray<FLSSessionItem>& Loadout);
	void MirrorRaidSessionState(const TArray<FLSSessionItem>& InventoryItems, const TArray<FLSSessionItem>& SafeItems);

	// 레이드 종료 - 결과 처리 후 결과 레벨로 전환
	UFUNCTION(BlueprintCallable, Category="LS/Session")
	void EndRaid(ELSRaidResult Result);

	// 파밍으로 획득한 아이템 추가
	UFUNCTION(BlueprintCallable, Category="LS/Session")
	void AddSessionItem(FName ItemRowName, int32 Amount);

	bool TryAddSessionItem(FName ItemRowName, int32 Amount, FLSSessionItem& OutRemainingItem);

	UFUNCTION(BlueprintCallable, Category="LS/Session")
	void SortSessionInventory();

	UFUNCTION(BlueprintCallable, Category="LS/Session")
	bool SwapSessionInventorySlots(int32 FromIndex, int32 ToIndex);

	UFUNCTION(BlueprintCallable, Category="LS/Session")
	bool MoveSessionInventorySlot(int32 FromIndex, int32 ToIndex);

	bool SwapSessionSlots(ELSInventorySlotArea FromArea, int32 FromIndex, ELSInventorySlotArea ToArea, int32 ToIndex);
	bool MoveSessionSlot(ELSInventorySlotArea FromArea, int32 FromIndex, ELSInventorySlotArea ToArea, int32 ToIndex);
	bool DropSessionSlot(ELSInventorySlotArea FromArea, int32 FromIndex, ELSInventorySlotArea ToArea, int32 ToIndex);
	bool DropExternalItemToSessionSlot(FLSSessionItem& InOutExternalItem, ELSInventorySlotArea ToArea, int32 ToIndex);
	bool GetSessionSlotItem(ELSInventorySlotArea SlotArea, int32 SlotIndex, FLSSessionItem& OutItem) const;
	bool ClearSessionSlot(ELSInventorySlotArea SlotArea, int32 SlotIndex);
	bool ReplaceSessionSlotItem(ELSInventorySlotArea SlotArea, int32 SlotIndex, const FLSSessionItem& NewItem, FLSSessionItem& OutPreviousItem);

	// 레이드 중 소모한 아이템 기록
	UFUNCTION(BlueprintCallable, Category="LS/Session")
	void ConsumeItem(FName ItemRowName, int32 Amount);

	UFUNCTION(BlueprintPure, Category="LS/Session")
	ELSRaidResult GetLastRaidResult() const { return LastRaidResult; }

	UFUNCTION(BlueprintPure, Category="LS/Session")
	const TArray<FLSSessionItem>& GetSessionInventory() const { return SessionInventory; }

	UFUNCTION(BlueprintPure, Category="LS/Session")
	const TArray<FLSSessionItem>& GetSessionSafeInventory() const { return SessionSafeInventory; }

	UFUNCTION(BlueprintPure, Category="LS/Session")
	bool IsRaidActive() const { return bRaidActive; }

	UFUNCTION(BlueprintPure, Category="LS/Session")
	int32 GetMaxInventorySlotCount() const;

	// 결과 레벨에서 최종 확정된 아이템 목록 조회 (EndRaid 이후 유효)
	UFUNCTION(BlueprintPure, Category="LS/Session")
	const TArray<FLSSessionItem>& GetResolvedItems() const { return ResolvedItems; }

private:
	bool bRaidActive = false;
	FLSLoadoutSnapshot LoadoutSnapshot;
	TArray<FLSSessionItem> SessionInventory;
	TArray<FLSSessionItem> SessionSafeInventory;
	TArray<FLSSessionItem> ConsumedItems;
	TArray<FLSSessionItem> ResolvedItems;
	ELSRaidResult LastRaidResult = ELSRaidResult::Dead;

	void StartRaidInternal(const TArray<FLSSessionItem>& Loadout, bool bPersistRaidSave);
	TArray<FLSSessionItem> BuildQuitRecovery() const;
};
