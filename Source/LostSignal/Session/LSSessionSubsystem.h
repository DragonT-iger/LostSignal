#pragma once

#include "CoreMinimal.h"
#include "Data/LSChipStats.h"
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
	// 무기/방어구 장착 슬롯. 로비 전용이며 SaveGame.EquipmentSlots를 원본으로 쓴다.
	Equipment,
};

// 장비 장착 슬롯 종류. 슬롯 인덱스가 곧 타입이며, EquipmentSlots 배열의 순서와 일치한다.
UENUM(BlueprintType)
enum class ELSEquipmentSlot : uint8
{
	Weapon,		// 무기 (Weapon_*)
	Processor,	// 머리 (Armor Item_Equipment=Processor)
	Core,		// 몸 (Armor Item_Equipment=Core)
	Actuator,	// 손 (Armor Item_Equipment=Actuator)
	Frame,		// 발 (Armor Item_Equipment=Frame)
	Count UMETA(Hidden),	// 슬롯 개수(=5). 장착 불가 판정에도 사용.
};

USTRUCT(BlueprintType)
struct FLSSessionItem
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FName ItemRowName;
	UPROPERTY(BlueprintReadOnly) int32 Amount = 0;

	// 칩 인스턴스 확정 전투 스탯 스냅샷. 비어 있으면 비칩.
	// 획득 시점에 1회 롤링해 고정한 값이라 데이터 테이블을 패치해도 변하지 않는다.
	// 이동/저장/복제 시 값 복사로 그대로 보존.
	UPROPERTY(BlueprintReadOnly) TArray<FLSChipResolvedStat> ChipStats;
};

// 레이드 입장 시점의 장비 스냅샷
USTRUCT(BlueprintType)
struct FLSLoadoutSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) TArray<FLSSessionItem> Items;
};

// PreLogin 거절 사유 표식. 서버가 ErrorMessage에 실어 보내면 클라의 네트워크 실패 문자열로 들어온다.
// 그대로 화면에 쓰지 않고(로컬라이징 대상이 아니다) 클라에서 FText 메시지로 바꿔 보여준다.
namespace LSNetRejectReason
{
	inline const TCHAR* RaidInProgress = TEXT("LS_RAID_IN_PROGRESS");
}

UCLASS()
class LOSTSIGNAL_API ULSSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// 탈주 시 출발 장비 복구 허용 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LS/Session")
	bool bAllowQuitRecovery = false;

	// 접속/이동 실패로 로비에 되돌아왔을 때 보여줄 사유. 로비 메뉴가 한 번 꺼내 가면 비워진다.
	// 실패는 여러 번 브로드캐스트되므로(체크섬 불일치는 액터 수만큼 뜬다) 첫 사유만 남긴다.
	bool ConsumePendingNetworkFailureMessage(FText& OutMessage);

	// 레이드 시작 - 스냅샷 찍고 세션 초기화
	UFUNCTION(BlueprintCallable, Category="LS/Session")
	void StartRaid(const TArray<FLSSessionItem>& Loadout);

	void ClearRaidSessionState();

	// 파밍으로 획득한 아이템 추가
	UFUNCTION(BlueprintCallable, Category="LS/Session")
	void AddSessionItem(FName ItemRowName, int32 Amount);

	bool TryAddSessionItem(FName ItemRowName, int32 Amount, const TArray<FLSChipResolvedStat>& ChipStats, FLSSessionItem& OutRemainingItem);

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
	const TArray<FLSSessionItem>& GetSessionInventory() const { return SessionInventory; }

	UFUNCTION(BlueprintPure, Category="LS/Session")
	const TArray<FLSSessionItem>& GetSessionSafeInventory() const { return SessionSafeInventory; }

	UFUNCTION(BlueprintPure, Category="LS/Session")
	bool IsRaidActive() const { return bRaidActive; }

	UFUNCTION(BlueprintPure, Category="LS/Session")
	int32 GetMaxInventorySlotCount() const;

	UFUNCTION(BlueprintPure, Category="LS/Session")
	int32 GetMaxSafeSlotCount() const;

private:
	bool bRaidActive = false;
	FLSLoadoutSnapshot LoadoutSnapshot;
	TArray<FLSSessionItem> SessionInventory;
	TArray<FLSSessionItem> SessionSafeInventory;
	TArray<FLSSessionItem> ConsumedItems;

	void StartRaidInternal(const TArray<FLSSessionItem>& Loadout, bool bPersistRaidSave);

	// 접속이 끊기면 엔진 기본값은 GameDefaultMap(타이틀)으로 나가는 것이다. 그러면 로비에서 하던
	// 장비·창고 작업이 통째로 날아간 것처럼 보이므로, 자기 로비(리슨 서버)로 되돌린다.
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
	void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);
	void ReturnToLobbyAfterFailure(const FText& Reason, const FString& LogContext);
	void HandlePostLoadMap(UWorld* LoadedWorld);

	FText PendingNetworkFailureMessage;

	// 실패 브로드캐스트가 연달아 오므로 첫 번째만 처리한다. 로비 로드가 끝나면 풀린다.
	bool bReturningToLobbyAfterFailure = false;
};
