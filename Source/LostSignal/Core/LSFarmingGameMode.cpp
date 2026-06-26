#include "Core/LSFarmingGameMode.h"
#include "Core/LSPlayerControllerBase.h"
#include "Inventory/LSRaidInventoryComponent.h"
#include "Kismet/GameplayStatics.h"
#include "LostSignal.h"
#include "Session/LSSaveSubsystem.h"
#include "Session/LSSessionSettings.h"
#include "Session/LSSessionSubsystem.h"

namespace
{
constexpr float RaidResultSaveTimeoutSeconds = 10.0f;
constexpr float SignalGaugeDrainIntervalSeconds = 60.0f;
constexpr float SignalGaugeDrainStepPercent = 0.1f;
}

void ALSFarmingGameMode::StartPlay()
{
	Super::StartPlay();

	// 레이드 진입 시 신호 게이지를 가득 채운 뒤 시간 감소를 시작한다.
	if (ULSSaveSubsystem* SaveSubsystem = GetSaveSubsystem())
	{
		SaveSubsystem->SetChipSignalGaugePercent(1.0f);
	}

	StartSignalGaugeDrain();
}

void ALSFarmingGameMode::OnPlayerDied()
{
	EndRaid(ELSRaidResult::Dead);
}

void ALSFarmingGameMode::OnExtraction()
{
	EndRaid(ELSRaidResult::Extracted);
}

void ALSFarmingGameMode::OnQuit()
{
	EndRaid(ELSRaidResult::Quit);
}

void ALSFarmingGameMode::NotifyRaidResultSaved(ALSPlayerControllerBase* PlayerController)
{
	if (!bWaitingForRaidResultSave)
	{
		return;
	}

	PendingRaidResultSaveControllers.Remove(PlayerController);
	UE_LOG(LogLS, Log, TEXT("[FarmingGameMode] Raid result save confirmed by %s. Remaining=%d"),
		*GetNameSafe(PlayerController),
		PendingRaidResultSaveControllers.Num());

	if (PendingRaidResultSaveControllers.IsEmpty())
	{
		TravelToResultLevel();
	}
}

void ALSFarmingGameMode::EndRaid(const ELSRaidResult Result)
{
	if (bRaidEnded)
	{
		return;
	}

	bRaidEnded = true;
	PendingRaidResult = Result;
	BeginRaidResultSave(Result);
}

void ALSFarmingGameMode::BeginRaidResultSave(const ELSRaidResult Result)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogLS, Warning, TEXT("[FarmingGameMode] Cannot end raid because World is missing."));
		bRaidEnded = false;
		return;
	}

	TArray<ALSPlayerControllerBase*> ResultControllers;
	for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(Iterator->Get());
		if (!PlayerController)
		{
			continue;
		}

		const ULSRaidInventoryComponent* RaidInventory = PlayerController->GetRaidInventoryComponent();
		if (!RaidInventory || !RaidInventory->IsRaidActive())
		{
			continue;
		}

		ResultControllers.Add(PlayerController);
		PendingRaidResultSaveControllers.Add(PlayerController);
	}

	if (ResultControllers.IsEmpty())
	{
		UE_LOG(LogLS, Warning, TEXT("[FarmingGameMode] Cannot end raid because no active raid inventories were found."));
		ClearRaidResultSaveWait();
		bRaidEnded = false;
		return;
	}

	bWaitingForRaidResultSave = true;
	GetWorldTimerManager().SetTimer(
		RaidResultSaveTimeoutTimerHandle,
		this,
		&ALSFarmingGameMode::HandleRaidResultSaveTimeout,
		RaidResultSaveTimeoutSeconds,
		false);

	for (ALSPlayerControllerBase* PlayerController : ResultControllers)
	{
		TArray<FLSSessionItem> InventoryItems;
		TArray<FLSSessionItem> SafeItems;
		bool bSaveInventory = false;
		bool bSaveSafeStash = false;

		if (!BuildRaidResultForPlayer(PlayerController, Result, InventoryItems, SafeItems, bSaveInventory, bSaveSafeStash))
		{
			UE_LOG(LogLS, Warning, TEXT("[FarmingGameMode] Failed to build raid result for %s."), *GetNameSafe(PlayerController));
			continue;
		}

		PlayerController->RequestRaidResultSave(Result, InventoryItems, SafeItems, bSaveInventory, bSaveSafeStash);
	}
}

bool ALSFarmingGameMode::BuildRaidResultForPlayer(
	const ALSPlayerControllerBase* PlayerController,
	const ELSRaidResult Result,
	TArray<FLSSessionItem>& OutInventoryItems,
	TArray<FLSSessionItem>& OutSafeItems,
	bool& bOutSaveInventory,
	bool& bOutSaveSafeStash) const
{
	OutInventoryItems.Reset();
	OutSafeItems.Reset();
	bOutSaveInventory = false;
	bOutSaveSafeStash = false;

	if (!PlayerController)
	{
		return false;
	}

	const ULSRaidInventoryComponent* RaidInventory = PlayerController->GetRaidInventoryComponent();
	if (!RaidInventory || !RaidInventory->IsRaidActive())
	{
		return false;
	}

	const ULSSessionSubsystem* SessionSub = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULSSessionSubsystem>() : nullptr;
	switch (Result)
	{
	case ELSRaidResult::Extracted:
		OutInventoryItems = RaidInventory->GetSessionInventory();
		OutSafeItems = RaidInventory->GetSessionSafeInventory();
		bOutSaveInventory = true;
		bOutSaveSafeStash = true;
		break;

	case ELSRaidResult::Dead:
		OutSafeItems = RaidInventory->GetSessionSafeInventory();
		bOutSaveInventory = true;
		bOutSaveSafeStash = true;
		break;

	case ELSRaidResult::Quit:
		if (SessionSub && SessionSub->bAllowQuitRecovery)
		{
			OutInventoryItems = PlayerController->GetSubmittedRaidLoadout();
		}
		bOutSaveInventory = true;
		bOutSaveSafeStash = false;
		break;

	default:
		return false;
	}

	return true;
}

void ALSFarmingGameMode::HandleRaidResultSaveTimeout()
{
	if (!bWaitingForRaidResultSave)
	{
		return;
	}

	for (const ALSPlayerControllerBase* PendingController : PendingRaidResultSaveControllers)
	{
		UE_LOG(LogLS, Warning, TEXT("[FarmingGameMode] Timed out waiting for raid result save ACK from %s."), *GetNameSafe(PendingController));
	}

	ClearRaidResultSaveWait();
	bRaidEnded = false;
}

void ALSFarmingGameMode::TravelToResultLevel()
{
	// 레이드 종료 — 신호 게이지 감소를 멈추고 로비 복귀를 위해 가득 채운다.
	StopSignalGaugeDrain();
	if (ULSSaveSubsystem* SaveSubsystem = GetSaveSubsystem())
	{
		SaveSubsystem->SetChipSignalGaugePercent(1.0f);
	}

	UWorld* World = GetWorld();
	if (World)
	{
		for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			if (ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(Iterator->Get()))
			{
				if (ULSRaidInventoryComponent* RaidInventory = PlayerController->GetRaidInventoryComponent())
				{
					RaidInventory->EndRaidInventory();
				}
				PlayerController->ClearSubmittedRaidEntryData();
			}
		}
	}

	if (ULSSessionSubsystem* SessionSub = GetGameInstance() ? GetGameInstance()->GetSubsystem<ULSSessionSubsystem>() : nullptr)
	{
		SessionSub->ClearRaidSessionState();
	}

	ClearRaidResultSaveWait();

	const ULSSessionSettings* Settings = GetDefault<ULSSessionSettings>();

	// 탈출 성공은 일단 ResultLevel을 건너뛰고 바로 로비로 복귀한다.
	if (PendingRaidResult == ELSRaidResult::Extracted)
	{
		if (Settings && !Settings->LobbyLevel.IsNull())
		{
			UGameplayStatics::OpenLevelBySoftObjectPtr(this, Settings->LobbyLevel);
		}
		else
		{
			UE_LOG(LogLS, Warning, TEXT("[FarmingGameMode] LobbyLevel is not set. Check Project Settings > LS Session Settings."));
		}
		return;
	}

	if (Settings && !Settings->ResultLevel.IsNull())
	{
		UGameplayStatics::OpenLevelBySoftObjectPtr(this, Settings->ResultLevel);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("[FarmingGameMode] ResultLevel is not set. Check Project Settings > LS Session Settings."));
	}
}

void ALSFarmingGameMode::ClearRaidResultSaveWait()
{
	bWaitingForRaidResultSave = false;
	PendingRaidResultSaveControllers.Reset();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RaidResultSaveTimeoutTimerHandle);
	}
}

void ALSFarmingGameMode::StartSignalGaugeDrain()
{
	GetWorldTimerManager().SetTimer(
		SignalGaugeDrainTimerHandle,
		this,
		&ALSFarmingGameMode::TickSignalGaugeDrain,
		SignalGaugeDrainIntervalSeconds,
		true);
}

void ALSFarmingGameMode::TickSignalGaugeDrain()
{
	ULSSaveSubsystem* SaveSubsystem = GetSaveSubsystem();
	if (!SaveSubsystem)
	{
		UE_LOG(LogLS, Warning, TEXT("[FarmingGameMode] Cannot drain signal gauge because SaveSubsystem is missing."));
		StopSignalGaugeDrain();
		return;
	}

	const float NextPercent = FMath::Max(SaveSubsystem->GetChipSignalGaugePercent() - SignalGaugeDrainStepPercent, 0.0f);
	SaveSubsystem->SetChipSignalGaugePercent(NextPercent);

	if (NextPercent <= 0.0f)
	{
		StopSignalGaugeDrain();
	}
}

void ALSFarmingGameMode::StopSignalGaugeDrain()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SignalGaugeDrainTimerHandle);
	}
}

ULSSaveSubsystem* ALSFarmingGameMode::GetSaveSubsystem() const
{
	UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
}
