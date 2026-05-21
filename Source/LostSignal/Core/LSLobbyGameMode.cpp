#include "Core/LSLobbyGameMode.h"

#include "Core/LSPlayerControllerBase.h"
#include "Inventory/LSRaidInventoryComponent.h"
#include "LostSignal.h"
#include "Session/LSSaveSubsystem.h"
#include "Session/LSSessionSettings.h"
#include "Session/LSSessionSubsystem.h"

void ALSLobbyGameMode::StartRaid()
{
	if (bRaidStartRequested)
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	ULSSessionSubsystem* SessionSub = GameInstance ? GameInstance->GetSubsystem<ULSSessionSubsystem>() : nullptr;
	if (!SessionSub)
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] Cannot start raid because SessionSubsystem is missing."));
		return;
	}

	const ULSSessionSettings* Settings = GetDefault<ULSSessionSettings>();
	if (!Settings || Settings->FarmingLevel.IsNull())
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] FarmingLevel is not set. Check Project Settings > LS Session Settings."));
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] Cannot start raid because World is missing."));
		return;
	}

	const FString FarmingLevelPath = Settings->FarmingLevel.ToSoftObjectPath().GetLongPackageName();
	if (FarmingLevelPath.IsEmpty())
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] Cannot start raid because FarmingLevel path is invalid."));
		return;
	}

	bRaidStartRequested = true;

	TArray<FLSSessionItem> Loadout;
	TArray<FLSSessionItem> SafeItems;
	const ULSSaveSubsystem* SaveSub = GameInstance->GetSubsystem<ULSSaveSubsystem>();
	if (SaveSub)
	{
		Loadout = SaveSub->GetInventory();
		SafeItems = SaveSub->GetSafeStash();
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] SaveSubsystem is missing. Starting raid with an empty inventory."));
	}

	SessionSub->StartRaid(Loadout);

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(It->Get()))
		{
			if (ULSRaidInventoryComponent* RaidInventory = PlayerController->GetRaidInventoryComponent())
			{
				RaidInventory->StartRaidInventory(Loadout, SafeItems);
			}
			PlayerController->ClientStartRaidSession(Loadout, SafeItems);
		}
	}

	if (!World->ServerTravel(FarmingLevelPath))
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] Failed to server travel to raid level: %s"), *FarmingLevelPath);
		bRaidStartRequested = false;
	}
}
