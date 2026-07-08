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
	// 이미 종료됐거나 사망 딜레이가 진행 중이면 무시 (중복 사망 가드)
	if (bRaidEnded || GetWorldTimerManager().IsTimerActive(DeathRaidEndTimerHandle))
	{
		return;
	}

	// 사망 연출을 보여준 뒤 결과 저장·레벨 이동을 시작한다.
	if (DeathRaidEndDelaySeconds <= 0.0f)
	{
		EndRaid(ELSRaidResult::Dead);
		return;
	}

	FTimerDelegate DeathRaidEndDelegate = FTimerDelegate::CreateUObject(this, &ALSFarmingGameMode::EndRaid, ELSRaidResult::Dead);
	GetWorldTimerManager().SetTimer(DeathRaidEndTimerHandle, DeathRaidEndDelegate, DeathRaidEndDelaySeconds, false);
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
		TArray<FLSSessionItem> EquipmentItems;
		bool bSaveInventory = false;
		bool bSaveSafeStash = false;
		bool bSaveEquipment = false;

		if (!BuildRaidResultForPlayer(PlayerController, Result, InventoryItems, SafeItems, EquipmentItems, bSaveInventory, bSaveSafeStash, bSaveEquipment))
		{
			UE_LOG(LogLS, Warning, TEXT("[FarmingGameMode] Failed to build raid result for %s."), *GetNameSafe(PlayerController));
			continue;
		}

		PlayerController->RequestRaidResultSave(Result, InventoryItems, SafeItems, EquipmentItems, bSaveInventory, bSaveSafeStash, bSaveEquipment);
	}
}

bool ALSFarmingGameMode::BuildRaidResultForPlayer(
	const ALSPlayerControllerBase* PlayerController,
	const ELSRaidResult Result,
	TArray<FLSSessionItem>& OutInventoryItems,
	TArray<FLSSessionItem>& OutSafeItems,
	TArray<FLSSessionItem>& OutEquipmentItems,
	bool& bOutSaveInventory,
	bool& bOutSaveSafeStash,
	bool& bOutSaveEquipment) const
{
	OutInventoryItems.Reset();
	OutSafeItems.Reset();
	OutEquipmentItems.Reset();
	bOutSaveInventory = false;
	bOutSaveSafeStash = false;
	bOutSaveEquipment = false;

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
		// 탈출: 레이드 최종 장착 상태를 세이브에 저장한다.
		OutEquipmentItems = RaidInventory->GetSessionEquipmentSlots();
		bOutSaveInventory = true;
		bOutSaveSafeStash = true;
		bOutSaveEquipment = true;
		break;

	case ELSRaidResult::Dead:
		OutSafeItems = RaidInventory->GetSessionSafeInventory();
		bOutSaveInventory = true;
		bOutSaveSafeStash = true;
		// 사망: 장비 소멸. 빈 배열을 저장하면 ReplaceEquipmentSlots가 빈 5칸으로 패딩한다(바닥 드랍 없음).
		bOutSaveEquipment = true;
		break;

	case ELSRaidResult::Quit:
		if (SessionSub && SessionSub->bAllowQuitRecovery)
		{
			OutInventoryItems = PlayerController->GetSubmittedRaidLoadout();
		}
		bOutSaveInventory = true;
		bOutSaveSafeStash = false;
		// Quit/강제종료: 장비는 저장하지 않는다. 레이드 중 클라 SaveGame.EquipmentSlots는 아무도 건드리지 않으므로
		// "저장 생략 = 입장 시점 장착 상태로 자동 복구"가 성립한다. (인벤의 bAllowQuitRecovery와 규칙이 다름)
		bOutSaveEquipment = false;
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

	// 탈출 성공·사망은 일단 ResultLevel을 건너뛰고 바로 로비로 복귀한다. (결과 레벨이 준비되면 되돌린다)
	if (PendingRaidResult == ELSRaidResult::Extracted || PendingRaidResult == ELSRaidResult::Dead)
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

	// 중도 포기(Quit)는 레이드 자체를 그만두는 행동이라 ResultLevel을 건너뛰고 바로 타이틀로 나간다.
	if (PendingRaidResult == ELSRaidResult::Quit)
	{
		if (Settings && !Settings->TitleLevel.IsNull())
		{
			UGameplayStatics::OpenLevelBySoftObjectPtr(this, Settings->TitleLevel);
		}
		else
		{
			UE_LOG(LogLS, Warning, TEXT("[FarmingGameMode] TitleLevel is not set. Check Project Settings > LS Session Settings."));
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
	const float Speed = FMath::Max(SignalGaugeDrainDebugSpeed, KINDA_SMALL_NUMBER);
	const float Interval = FMath::Max(SignalGaugeDrainIntervalSeconds / Speed, 0.01f);
	GetWorldTimerManager().SetTimer(
		SignalGaugeDrainTimerHandle,
		this,
		&ALSFarmingGameMode::TickSignalGaugeDrain,
		Interval,
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

float ALSFarmingGameMode::GetSignalGaugeDrainRemainingSeconds() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return -1.0f;
	}

	// 비활성/정지 상태면 음수를 반환하므로 호출부가 폴백을 판단할 수 있다.
	return World->GetTimerManager().GetTimerRemaining(SignalGaugeDrainTimerHandle);
}

float ALSFarmingGameMode::GetSignalGaugeDrainInterval() const
{
	// 배속이 걸리면 실제 타이머 주기도 짧아지므로, 링 카운트다운 분모도 같은 값을 써야 비율이 맞다.
	const float Speed = FMath::Max(SignalGaugeDrainDebugSpeed, KINDA_SMALL_NUMBER);
	return SignalGaugeDrainIntervalSeconds / Speed;
}

void ALSFarmingGameMode::SetSignalGaugeDrainDebugSpeed(const float Speed)
{
	SignalGaugeDrainDebugSpeed = FMath::Clamp(Speed, 0.01f, 100.0f);

	// 드레인이 진행 중이면 새 배속 주기로 타이머를 다시 건다(남은 시간은 새 주기 기준으로 리셋).
	if (GetWorldTimerManager().IsTimerActive(SignalGaugeDrainTimerHandle))
	{
		StartSignalGaugeDrain();
	}
}

ULSSaveSubsystem* ALSFarmingGameMode::GetSaveSubsystem() const
{
	UGameInstance* GameInstance = GetGameInstance();
	return GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
}
