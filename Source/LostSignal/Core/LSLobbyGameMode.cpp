#include "Core/LSLobbyGameMode.h"

#include "Core/LSPlayerControllerBase.h"
#include "Inventory/LSRaidInventoryComponent.h"
#include "LostSignal.h"
#include "Session/LSSessionSettings.h"
#include "Session/LSSessionSubsystem.h"

namespace
{
constexpr float RaidEntryDataTimeoutSeconds = 10.0f;
}

void ALSLobbyGameMode::StartRaid()
{
	if (bRaidStartRequested)
	{
		return;
	}

	bRaidStartRequested = true;
	bWaitingForRaidEntryData = true;
	GetWorldTimerManager().SetTimer(
		RaidEntryDataTimeoutTimerHandle,
		this,
		&ALSLobbyGameMode::HandleRaidEntryDataTimeout,
		RaidEntryDataTimeoutSeconds,
		false);

	if (!RequestRaidEntryDataFromPlayers())
	{
		ClearRaidEntryDataWait();
		return;
	}

	TryStartRaidWithSubmittedData();
}

void ALSLobbyGameMode::NotifyRaidEntryDataSubmitted(ALSPlayerControllerBase* PlayerController)
{
	if (!bWaitingForRaidEntryData)
	{
		return;
	}

	UE_LOG(LogLS, Log, TEXT("[Lobby] Raid entry data received from %s."), *GetNameSafe(PlayerController));
	TryStartRaidWithSubmittedData();
}

bool ALSLobbyGameMode::RequestRaidEntryDataFromPlayers()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] Cannot request raid entry data because World is missing."));
		return false;
	}

	bool bRequestedAnyPlayer = false;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(It->Get()))
		{
			bRequestedAnyPlayer = true;
			PlayerController->RequestRaidEntryDataForRaidStart();
		}
	}

	if (!bRequestedAnyPlayer)
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] Cannot start raid because there are no LS player controllers."));
	}

	return bRequestedAnyPlayer;
}

bool ALSLobbyGameMode::AreRaidEntryDataReady() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	bool bFoundPlayer = false;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(It->Get());
		if (!PlayerController)
		{
			continue;
		}

		bFoundPlayer = true;
		if (!PlayerController->HasSubmittedRaidEntryData())
		{
			return false;
		}
	}

	return bFoundPlayer;
}

void ALSLobbyGameMode::TryStartRaidWithSubmittedData()
{
	if (!bRaidStartRequested || !bWaitingForRaidEntryData || !AreRaidEntryDataReady())
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	ULSSessionSubsystem* SessionSub = GameInstance ? GameInstance->GetSubsystem<ULSSessionSubsystem>() : nullptr;
	if (!SessionSub)
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] Cannot start raid because SessionSubsystem is missing."));
		ClearRaidEntryDataWait();
		return;
	}

	const ULSSessionSettings* Settings = GetDefault<ULSSessionSettings>();
	if (!Settings || Settings->FarmingLevel.IsNull())
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] FarmingLevel is not set. Check Project Settings > LS Session Settings."));
		ClearRaidEntryDataWait();
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] Cannot start raid because World is missing."));
		ClearRaidEntryDataWait();
		return;
	}

	const FString FarmingLevelPath = Settings->FarmingLevel.ToSoftObjectPath().GetLongPackageName();
	if (FarmingLevelPath.IsEmpty())
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] Cannot start raid because FarmingLevel path is invalid."));
		ClearRaidEntryDataWait();
		return;
	}

	TArray<FLSSessionItem> Loadout;
	TArray<FLSSessionItem> SafeItems;
	bool bHasLegacySessionLoadout = false;

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(It->Get()))
		{
			const TArray<FLSSessionItem>& PlayerLoadout = PlayerController->GetSubmittedRaidLoadout();
			const TArray<FLSSessionItem>& PlayerSafeItems = PlayerController->GetSubmittedRaidSafeItems();
			if (!bHasLegacySessionLoadout)
			{
				Loadout = PlayerLoadout;
				SafeItems = PlayerSafeItems;
				bHasLegacySessionLoadout = true;
			}

			// ServerTravel 이후 각 PC가 자신의 데이터를 꺼낼 수 있도록 순서대로 큐에 저장
			SessionSub->EnqueuePendingRaidEntry(PlayerLoadout, PlayerSafeItems);

			if (ULSRaidInventoryComponent* RaidInventory = PlayerController->GetRaidInventoryComponent())
			{
				RaidInventory->StartRaidInventory(PlayerLoadout, PlayerSafeItems);
			}
			PlayerController->ClientStartRaidSession(PlayerLoadout, PlayerSafeItems);
		}
	}

	SessionSub->StartRaid(Loadout);
	SessionSub->MirrorRaidSessionState(Loadout, SafeItems);
	ClearRaidEntryDataWait();
	bRaidStartRequested = true;

	if (!World->ServerTravel(FarmingLevelPath))
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] Failed to server travel to raid level: %s"), *FarmingLevelPath);
		ClearRaidEntryDataWait();
	}
}

void ALSLobbyGameMode::HandleRaidEntryDataTimeout()
{
	if (!bWaitingForRaidEntryData)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			const ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(It->Get());
			if (PlayerController && !PlayerController->HasSubmittedRaidEntryData())
			{
				UE_LOG(LogLS, Warning, TEXT("[Lobby] Timed out waiting for raid entry data from %s."), *GetNameSafe(PlayerController));
			}
		}
	}

	ClearRaidEntryDataWait();
}

void ALSLobbyGameMode::ClearRaidEntryDataWait()
{
	bRaidStartRequested = false;
	bWaitingForRaidEntryData = false;
	GetWorldTimerManager().ClearTimer(RaidEntryDataTimeoutTimerHandle);
}
