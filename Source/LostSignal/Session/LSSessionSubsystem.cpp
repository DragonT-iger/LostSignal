#include "Session/LSSessionSubsystem.h"
#include "Session/LSSaveSubsystem.h"
#include "Session/LSSessionSettings.h"
#include "Engine/Engine.h"
#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "Inventory/LSInventorySlotUtils.h"
#include "Kismet/GameplayStatics.h"
#include "LostSignal.h"
#include "UObject/UObjectGlobals.h"

#define LOCTEXT_NAMESPACE "LSSession"

namespace
{
constexpr int32 SessionDefaultMaxInventorySlotCount = 10;
constexpr int32 SessionDefaultMaxSafeSlotCount = 4;
}

void ULSSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (GEngine)
	{
		GEngine->OnNetworkFailure().AddUObject(this, &ULSSessionSubsystem::HandleNetworkFailure);
		GEngine->OnTravelFailure().AddUObject(this, &ULSSessionSubsystem::HandleTravelFailure);
	}
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ULSSessionSubsystem::HandlePostLoadMap);
}

void ULSSessionSubsystem::Deinitialize()
{
	if (GEngine)
	{
		GEngine->OnNetworkFailure().RemoveAll(this);
		GEngine->OnTravelFailure().RemoveAll(this);
	}
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	Super::Deinitialize();
}

void ULSSessionSubsystem::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, const ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	// 실패한 드라이버가 클라이언트일 때만 이동한다. 호스트는 참가자가 나가도 자기 세션을 유지해야 한다.
	// (엔진도 UEngine::HandleNetworkFailure에서 같은 기준으로 bShouldTravel을 정한다)
	if (!NetDriver || NetDriver->GetNetMode() != NM_Client)
	{
		UE_LOG(LogLS, Log, TEXT("[Session] Ignoring a network failure that is not ours to travel on. Type=%s Error=%s"),
			ENetworkFailure::ToString(FailureType), *ErrorString);
		return;
	}

	// 체크섬 불일치는 액터 수만큼 브로드캐스트된다. 첫 사유만 남기고 나머지는 무시한다.
	const FText Reason = (FailureType == ENetworkFailure::NetChecksumMismatch)
		? LOCTEXT("NetChecksumMismatch", "호스트와 <Emph>게임 버전</>이 달라 접속이 끊겼습니다. 양쪽 모두 같은 빌드인지 확인해 주세요.")
		: LOCTEXT("NetworkFailure", "호스트와의 <Emph>접속이 끊겼습니다</>.");

	ReturnToLobbyAfterFailure(Reason, FString::Printf(TEXT("NetworkFailure=%s Error=%s"), ENetworkFailure::ToString(FailureType), *ErrorString));
}

void ULSSessionSubsystem::HandleTravelFailure(UWorld* World, const ETravelFailure::Type FailureType, const FString& ErrorString)
{
	ReturnToLobbyAfterFailure(
		LOCTEXT("TravelFailure", "호스트에 <Emph>접속하지 못했습니다</>. 주소와 네트워크를 확인해 주세요."),
		FString::Printf(TEXT("TravelFailure=%s Error=%s"), ETravelFailure::ToString(FailureType), *ErrorString));
}

void ULSSessionSubsystem::ReturnToLobbyAfterFailure(const FText& Reason, const FString& LogContext)
{
	if (bReturningToLobbyAfterFailure)
	{
		return;
	}

	UE_LOG(LogLS, Warning, TEXT("[Session] Returning to the lobby after a network problem. %s"), *LogContext);

	const ULSSessionSettings* Settings = GetDefault<ULSSessionSettings>();
	const FString LobbyMapName = (Settings && !Settings->LobbyLevel.IsNull())
		? Settings->LobbyLevel.ToSoftObjectPath().GetLongPackageName()
		: FString();
	if (LobbyMapName.IsEmpty())
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] LobbyLevel is not set. Check Project Settings > LS Session Settings."));
		return;
	}

	bReturningToLobbyAfterFailure = true;
	PendingNetworkFailureMessage = Reason;

	// 레이드 상태를 들고 로비로 돌아가면 인벤토리 표시가 어긋나므로 먼저 정리한다.
	ClearRaidSessionState();

	// 다시 자기 방(리슨 서버)으로 돌아간다. 타이틀로 나가면 로비 작업이 통째로 날아간 것처럼 보인다.
	UGameplayStatics::OpenLevel(this, FName(*LobbyMapName), true, TEXT("listen"));
}

void ULSSessionSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	// 로비 로드가 끝났으니 다음 실패를 다시 받을 수 있게 푼다.
	bReturningToLobbyAfterFailure = false;
}

bool ULSSessionSubsystem::ConsumePendingNetworkFailureMessage(FText& OutMessage)
{
	if (PendingNetworkFailureMessage.IsEmpty())
	{
		return false;
	}

	OutMessage = PendingNetworkFailureMessage;
	PendingNetworkFailureMessage = FText::GetEmpty();
	return true;
}

void ULSSessionSubsystem::StartRaid(const TArray<FLSSessionItem>& Loadout)
{
	StartRaidInternal(Loadout, true);
}

void ULSSessionSubsystem::ClearRaidSessionState()
{
	LoadoutSnapshot.Items.Reset();
	SessionInventory.Reset();
	SessionSafeInventory.Reset();
	ConsumedItems.Reset();
	bRaidActive = false;
}

void ULSSessionSubsystem::StartRaidInternal(const TArray<FLSSessionItem>& Loadout, const bool bPersistRaidSave)
{
	LoadoutSnapshot.Items = Loadout;
	SessionInventory = Loadout;
	SessionSafeInventory.Empty();
	ConsumedItems.Empty();
	bRaidActive = true;

	if (ULSSaveSubsystem* SaveSub = GetGameInstance()->GetSubsystem<ULSSaveSubsystem>())
	{
		SessionSafeInventory = SaveSub->GetSafeStash();
		if (bPersistRaidSave)
		{
			SaveSub->BeginRaidSave(LoadoutSnapshot.Items);
		}
	}

	UE_LOG(LogLS, Log, TEXT("[Session] Raid started with %d inventory slots."), SessionInventory.Num());
}

void ULSSessionSubsystem::AddSessionItem(FName ItemRowName, int32 Amount)
{
	FLSSessionItem IgnoredRemainingItem;
	TryAddSessionItem(ItemRowName, Amount, /*ChipStats=*/TArray<FLSChipResolvedStat>(), IgnoredRemainingItem);
}

bool ULSSessionSubsystem::TryAddSessionItem(FName ItemRowName, int32 Amount, const TArray<FLSChipResolvedStat>& ChipStats, FLSSessionItem& OutRemainingItem)
{
	return LSInventorySlotUtils::TryAddItemsToSlotArray(SessionInventory, ItemRowName, Amount, GetMaxInventorySlotCount(), ChipStats, OutRemainingItem);
}

void ULSSessionSubsystem::SortSessionInventory()
{
	LSInventorySlotUtils::SortAndCompactSlotArray(SessionInventory);
	UE_LOG(LogLS, Log, TEXT("[Session] Session inventory sorted and compacted. Total slots: %d"), SessionInventory.Num());
}

bool ULSSessionSubsystem::SwapSessionInventorySlots(const int32 FromIndex, const int32 ToIndex)
{
	return SwapSessionSlots(ELSInventorySlotArea::Inventory, FromIndex, ELSInventorySlotArea::Inventory, ToIndex);
}

bool ULSSessionSubsystem::MoveSessionInventorySlot(const int32 FromIndex, const int32 ToIndex)
{
	return MoveSessionSlot(ELSInventorySlotArea::Inventory, FromIndex, ELSInventorySlotArea::Inventory, ToIndex);
}

bool ULSSessionSubsystem::SwapSessionSlots(const ELSInventorySlotArea FromArea, const int32 FromIndex, const ELSInventorySlotArea ToArea, const int32 ToIndex)
{
	if (FromArea == ELSInventorySlotArea::Warehouse || ToArea == ELSInventorySlotArea::Warehouse)
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] Warehouse slots are not part of raid session inventory."));
		return false;
	}

	TArray<FLSSessionItem>* FromSlots = FromArea == ELSInventorySlotArea::Safe ? &SessionSafeInventory : &SessionInventory;
	TArray<FLSSessionItem>* ToSlots = ToArea == ELSInventorySlotArea::Safe ? &SessionSafeInventory : &SessionInventory;

	if (!FromSlots->IsValidIndex(FromIndex) || !LSInventorySlotUtils::IsFilled((*FromSlots)[FromIndex]) || ToIndex < 0)
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] Cannot swap slots. FromArea=%d From=%d ToArea=%d To=%d"),
			static_cast<int32>(FromArea), FromIndex, static_cast<int32>(ToArea), ToIndex);
		return false;
	}

	const int32 ToMaxSlotCount = ToArea == ELSInventorySlotArea::Inventory
		? GetMaxInventorySlotCount()
		: GetMaxSafeSlotCount();
	return LSInventorySlotUtils::SwapSlots(*FromSlots, FromIndex, *ToSlots, ToIndex, ToMaxSlotCount);
}

bool ULSSessionSubsystem::MoveSessionSlot(const ELSInventorySlotArea FromArea, const int32 FromIndex, const ELSInventorySlotArea ToArea, const int32 ToIndex)
{
	if (FromArea == ELSInventorySlotArea::Warehouse || ToArea == ELSInventorySlotArea::Warehouse)
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] Warehouse slots are not part of raid session inventory."));
		return false;
	}

	if (FromArea != ToArea)
	{
		return SwapSessionSlots(FromArea, FromIndex, ToArea, ToIndex);
	}

	TArray<FLSSessionItem>& Slots = FromArea == ELSInventorySlotArea::Safe ? SessionSafeInventory : SessionInventory;
	if (!Slots.IsValidIndex(FromIndex) || !LSInventorySlotUtils::IsFilled(Slots[FromIndex]))
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] Cannot move inventory slot because FromIndex is invalid: %d"), FromIndex);
		return false;
	}

	return LSInventorySlotUtils::MoveSlotWithinArray(Slots, FromIndex, ToIndex);
}

bool ULSSessionSubsystem::DropSessionSlot(const ELSInventorySlotArea FromArea, const int32 FromIndex, const ELSInventorySlotArea ToArea, const int32 ToIndex)
{
	if (FromArea == ELSInventorySlotArea::Warehouse || ToArea == ELSInventorySlotArea::Warehouse)
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] Warehouse slots are not part of raid session inventory."));
		return false;
	}

	TArray<FLSSessionItem>* FromSlots = FromArea == ELSInventorySlotArea::Safe ? &SessionSafeInventory : &SessionInventory;
	TArray<FLSSessionItem>* ToSlots = ToArea == ELSInventorySlotArea::Safe ? &SessionSafeInventory : &SessionInventory;

	if (!FromSlots->IsValidIndex(FromIndex) || !LSInventorySlotUtils::IsFilled((*FromSlots)[FromIndex]) || ToIndex < 0)
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] Cannot drop slot. FromArea=%d From=%d ToArea=%d To=%d"),
			static_cast<int32>(FromArea), FromIndex, static_cast<int32>(ToArea), ToIndex);
		return false;
	}

	const int32 ToMaxSlotCount = ToArea == ELSInventorySlotArea::Inventory
		? GetMaxInventorySlotCount()
		: GetMaxSafeSlotCount();
	return LSInventorySlotUtils::DropSlot(*FromSlots, FromIndex, *ToSlots, ToIndex, ToMaxSlotCount);
}

bool ULSSessionSubsystem::DropExternalItemToSessionSlot(FLSSessionItem& InOutExternalItem, const ELSInventorySlotArea ToArea, const int32 ToIndex)
{
	if (ToArea == ELSInventorySlotArea::Warehouse)
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] Warehouse slots are not part of raid session inventory."));
		return false;
	}

	TArray<FLSSessionItem>* ToSlots = ToArea == ELSInventorySlotArea::Safe ? &SessionSafeInventory : &SessionInventory;
	if (!LSInventorySlotUtils::IsFilled(InOutExternalItem) || ToIndex < 0)
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] Cannot drop external item. ToArea=%d To=%d Row=%s Amount=%d"),
			static_cast<int32>(ToArea),
			ToIndex,
			*InOutExternalItem.ItemRowName.ToString(),
			InOutExternalItem.Amount);
		return false;
	}

	const int32 ToMaxSlotCount = ToArea == ELSInventorySlotArea::Inventory
		? GetMaxInventorySlotCount()
		: GetMaxSafeSlotCount();
	return LSInventorySlotUtils::DropExternalItemToSlot(InOutExternalItem, *ToSlots, ToIndex, ToMaxSlotCount);
}

bool ULSSessionSubsystem::GetSessionSlotItem(const ELSInventorySlotArea SlotArea, const int32 SlotIndex, FLSSessionItem& OutItem) const
{
	if (SlotArea == ELSInventorySlotArea::Warehouse)
	{
		return false;
	}

	const TArray<FLSSessionItem>& Slots = SlotArea == ELSInventorySlotArea::Safe ? SessionSafeInventory : SessionInventory;
	if (!Slots.IsValidIndex(SlotIndex) || !LSInventorySlotUtils::IsFilled(Slots[SlotIndex]))
	{
		return false;
	}

	OutItem = Slots[SlotIndex];
	return true;
}

bool ULSSessionSubsystem::ClearSessionSlot(const ELSInventorySlotArea SlotArea, const int32 SlotIndex)
{
	if (SlotArea == ELSInventorySlotArea::Warehouse)
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] Warehouse slots are not part of raid session inventory."));
		return false;
	}

	TArray<FLSSessionItem>& Slots = SlotArea == ELSInventorySlotArea::Safe ? SessionSafeInventory : SessionInventory;
	if (!Slots.IsValidIndex(SlotIndex) || !LSInventorySlotUtils::IsFilled(Slots[SlotIndex]))
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] Cannot clear slot. Area=%d Index=%d"),
			static_cast<int32>(SlotArea), SlotIndex);
		return false;
	}

	Slots[SlotIndex] = LSInventorySlotUtils::MakeEmptyItem();
	return true;
}

bool ULSSessionSubsystem::ReplaceSessionSlotItem(const ELSInventorySlotArea SlotArea, const int32 SlotIndex, const FLSSessionItem& NewItem, FLSSessionItem& OutPreviousItem)
{
	OutPreviousItem = FLSSessionItem();
	if (SlotArea == ELSInventorySlotArea::Warehouse)
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] Warehouse slots are not part of raid session inventory."));
		return false;
	}

	if (SlotIndex < 0)
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] Cannot replace slot because index is invalid. Area=%d Index=%d"),
			static_cast<int32>(SlotArea), SlotIndex);
		return false;
	}

	if (SlotArea == ELSInventorySlotArea::Inventory && SlotIndex >= GetMaxInventorySlotCount())
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] Cannot replace inventory slot because index exceeds max. Index=%d"), SlotIndex);
		return false;
	}
	if (SlotArea == ELSInventorySlotArea::Safe && SlotIndex >= GetMaxSafeSlotCount())
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] Cannot replace safe slot because index exceeds max. Index=%d"), SlotIndex);
		return false;
	}

	TArray<FLSSessionItem>& Slots = SlotArea == ELSInventorySlotArea::Safe ? SessionSafeInventory : SessionInventory;
	LSInventorySlotUtils::EnsureSlotIndex(Slots, SlotIndex);
	OutPreviousItem = Slots[SlotIndex];
	Slots[SlotIndex] = NewItem;
	return true;
}

int32 ULSSessionSubsystem::GetMaxInventorySlotCount() const
{
	const ULSSaveSubsystem* SaveSub = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULSSaveSubsystem>() : nullptr;
	return SaveSub ? SaveSub->GetMaxInventorySlotCount() : SessionDefaultMaxInventorySlotCount;
}

int32 ULSSessionSubsystem::GetMaxSafeSlotCount() const
{
	const ULSSaveSubsystem* SaveSub = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULSSaveSubsystem>() : nullptr;
	return SaveSub ? SaveSub->GetMaxSafeStashSlotCount() : SessionDefaultMaxSafeSlotCount;
}

void ULSSessionSubsystem::ConsumeItem(FName ItemRowName, int32 Amount)
{
	if (ItemRowName.IsNone() || Amount <= 0) return;

	for (FLSSessionItem& Item : ConsumedItems)
	{
		if (Item.ItemRowName == ItemRowName)
		{
			Item.Amount += Amount;
			if (ULSSaveSubsystem* SaveSub = GetGameInstance()->GetSubsystem<ULSSaveSubsystem>())
			{
				SaveSub->UpdateRaidConsumedItems(ConsumedItems);
			}
			return;
		}
	}

	ConsumedItems.Add({ ItemRowName, Amount });
	if (ULSSaveSubsystem* SaveSub = GetGameInstance()->GetSubsystem<ULSSaveSubsystem>())
	{
		SaveSub->UpdateRaidConsumedItems(ConsumedItems);
	}
}

#undef LOCTEXT_NAMESPACE
