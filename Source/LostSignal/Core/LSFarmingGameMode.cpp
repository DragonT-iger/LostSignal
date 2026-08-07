#include "Core/LSFarmingGameMode.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Core/LSPlayerControllerBase.h"
#include "Data/LSChipStats.h"
#include "Inventory/LSRaidInventoryComponent.h"
#include "LostSignal.h"
#include "Session/LSSaveSubsystem.h"
#include "Session/LSSessionSettings.h"
#include "Session/LSSessionSubsystem.h"

namespace
{
constexpr float RaidResultSaveTimeoutSeconds = 10.0f;
constexpr float SignalGaugeDrainIntervalSeconds = 60.0f;
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

void ALSFarmingGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	if (!ErrorMessage.IsEmpty())
	{
		return;
	}

	// 레이드는 로비에서 입장 payload를 제출한 인원으로 고정된다. 난입을 받으면 그 플레이어만
	// 인벤토리 없이 돌아다니다 결과 저장에서도 빠진다. 조용히 망가지느니 여기서 거절한다.
	ErrorMessage = LSNetRejectReason::RaidInProgress;
	UE_LOG(LogLS, Log, TEXT("[FarmingGameMode] Rejected a join from %s because a raid is in progress."), *Address);
}

void ALSFarmingGameMode::Logout(AController* Exiting)
{
	ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(Exiting);
	if (PlayerController)
	{
		// 이탈자는 더 이상 ACK를 보내지 못한다. 남은 인원의 레벨 전환이 막히지 않도록 대기 목록에서 뺀다.
		const int32 RemovedQuitWaits = PendingQuitControllers.Remove(PlayerController);
		const int32 RemovedResultWaits = PendingRaidResultSaveControllers.Remove(PlayerController);
		if (RemovedQuitWaits > 0 || RemovedResultWaits > 0)
		{
			UE_LOG(LogLS, Warning, TEXT("[FarmingGameMode] %s left while a raid result save was pending. Dropping the wait."),
				*GetNameSafe(PlayerController));
		}
	}

	Super::Logout(Exiting);

	// 남은 사람이 전부 ACK를 끝낸 상태였다면 이탈로 비워진 지금 전환한다.
	if (bWaitingForRaidResultSave && PendingRaidResultSaveControllers.IsEmpty())
	{
		TravelToResultLevel();
	}
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

void ALSFarmingGameMode::QuitRaidForPlayer(ALSPlayerControllerBase* QuittingPlayer)
{
	if (!QuittingPlayer)
	{
		return;
	}

	// 호스트는 곧 서버라 혼자 빠질 수 없다. 전원 Quit으로 확정하고 파티째 로비로 돌아간다.
	if (QuittingPlayer->IsLocalPlayerController())
	{
		UE_LOG(LogLS, Warning, TEXT("[FarmingGameMode] Host quit the raid. Ending the raid for everyone."));
		OnQuit();
		return;
	}

	const ULSRaidInventoryComponent* RaidInventory = QuittingPlayer->GetRaidInventoryComponent();
	if (!RaidInventory || !RaidInventory->IsRaidActive())
	{
		SendPlayerToOwnLobby(QuittingPlayer);
		return;
	}

	TArray<FLSSessionItem> InventoryItems;
	TArray<FLSSessionItem> SafeItems;
	TArray<FLSSessionItem> EquipmentItems;
	bool bSaveInventory = false;
	bool bSaveSafeStash = false;
	bool bSaveEquipment = false;

	if (!BuildRaidResultForPlayer(QuittingPlayer, ELSRaidResult::Quit, InventoryItems, SafeItems, EquipmentItems, bSaveInventory, bSaveSafeStash, bSaveEquipment))
	{
		UE_LOG(LogLS, Warning, TEXT("[FarmingGameMode] Failed to build quit result for %s. Sending to the lobby without saving."), *GetNameSafe(QuittingPlayer));
		SendPlayerToOwnLobby(QuittingPlayer);
		return;
	}

	PendingQuitControllers.AddUnique(QuittingPlayer);
	QuittingPlayer->RequestRaidResultSave(ELSRaidResult::Quit, InventoryItems, SafeItems, EquipmentItems, bSaveInventory, bSaveSafeStash, bSaveEquipment);
}

void ALSFarmingGameMode::NotifyRaidResultSaved(ALSPlayerControllerBase* PlayerController)
{
	// 개인 이탈 ACK — 그 사람만 정리해서 자기 로비로 내보내고 남은 사람의 레이드는 그대로 둔다.
	if (PlayerController && PendingQuitControllers.Remove(PlayerController) > 0)
	{
		if (ULSRaidInventoryComponent* RaidInventory = PlayerController->GetRaidInventoryComponent())
		{
			RaidInventory->EndRaidInventory();
		}
		PlayerController->ClearSubmittedRaidEntryData();
		UE_LOG(LogLS, Log, TEXT("[FarmingGameMode] Quit result saved by %s. Sending to the lobby."), *GetNameSafe(PlayerController));
		SendPlayerToOwnLobby(PlayerController);
		// return하지 않는다 — 이탈 요청과 전원 종료가 겹쳤으면 이 사람 몫의 그룹 대기도 함께 풀어야
		// PendingRaidResultSaveControllers가 비고 레벨 전환이 진행된다.
	}

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
			// 결과를 못 만들었으면 저장 요청도 못 보낸다. 대기 목록에 남겨두면 ACK가 영영 오지 않아
			// 레벨 전환이 막히므로 여기서 같이 뺀다.
			UE_LOG(LogLS, Warning, TEXT("[FarmingGameMode] Failed to build raid result for %s. Dropping it from the save wait."), *GetNameSafe(PlayerController));
			PendingRaidResultSaveControllers.Remove(PlayerController);
			continue;
		}

		PlayerController->RequestRaidResultSave(Result, InventoryItems, SafeItems, EquipmentItems, bSaveInventory, bSaveSafeStash, bSaveEquipment);
	}

	// 전원이 build 실패로 빠졌으면 여기서 이미 비어 있다. 대기만 걸어두고 멈추지 않도록 바로 전환한다.
	if (bWaitingForRaidResultSave && PendingRaidResultSaveControllers.IsEmpty())
	{
		TravelToResultLevel();
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
	if (!Settings)
	{
		UE_LOG(LogLS, Warning, TEXT("[FarmingGameMode] Cannot leave the raid level because LS Session Settings is missing."));
		return;
	}

	// 레이드에서 나가는 곳은 언제나 로비다(탈출·사망·전원 Quit 모두). 타이틀로 나가는 건 로비의 몫이다.
	// 탈출 성공·사망은 일단 ResultLevel도 건너뛴다. (결과 레벨이 준비되면 아래 분기를 되돌린다)
	// 개인 이탈은 여기까지 오지 않는다 — QuitRaidForPlayer가 그 사람만 ClientTravel로 내보낸다.
	if (PendingRaidResult == ELSRaidResult::Extracted
		|| PendingRaidResult == ELSRaidResult::Dead
		|| PendingRaidResult == ELSRaidResult::Quit)
	{
		ServerTravelToLevel(Settings->LobbyLevel, TEXT("LobbyLevel"));
		return;
	}

	ServerTravelToLevel(Settings->ResultLevel, TEXT("ResultLevel"));
}

bool ALSFarmingGameMode::ServerTravelToLevel(const TSoftObjectPtr<UWorld>& Level, const TCHAR* LevelLabel)
{
	// OpenLevel(=UEngine::SetClientTravel)을 쓰면 안 된다. 넷드라이버를 파괴해 클라 전원이 끊기고,
	// LastURL에서 Listen 옵션까지 떼어내 호스트가 리슨 서버를 그만둔다. 그러면 두 번째 레이드부터
	// 멀티가 성립하지 않는다. ServerTravel은 세션과 참가자를 유지한 채 맵만 바꾼다.
	UWorld* World = GetWorld();
	if (!World || Level.IsNull())
	{
		UE_LOG(LogLS, Warning, TEXT("[FarmingGameMode] %s is not set. Check Project Settings > LS Session Settings."), LevelLabel);
		return false;
	}

	const FString LevelPath = Level.ToSoftObjectPath().GetLongPackageName();
	if (LevelPath.IsEmpty() || !World->ServerTravel(LevelPath))
	{
		UE_LOG(LogLS, Warning, TEXT("[FarmingGameMode] Failed to server travel to %s: %s"), LevelLabel, *LevelPath);
		return false;
	}

	return true;
}

void ALSFarmingGameMode::SendPlayerToOwnLobby(ALSPlayerControllerBase* PlayerController)
{
	const ULSSessionSettings* Settings = GetDefault<ULSSessionSettings>();
	const FString LobbyPath = (Settings && !Settings->LobbyLevel.IsNull())
		? Settings->LobbyLevel.ToSoftObjectPath().GetLongPackageName()
		: FString();
	if (!PlayerController || LobbyPath.IsEmpty())
	{
		UE_LOG(LogLS, Warning, TEXT("[FarmingGameMode] Cannot send %s to the lobby level. Check Project Settings > LS Session Settings."),
			*GetNameSafe(PlayerController));
		return;
	}

	// 개인 이탈이므로 서버 전체를 옮기지 않는다. 그 클라이언트만 접속을 끊고 자기 로비를 로컬로 연다.
	// 호스트 세션에 남아 있을 수는 없다 — 접속 상태에서는 서버와 같은 맵에 있어야 하기 때문이다.
	// ?listen을 붙여 돌아온 로비에서 바로 다시 친구를 받을 수 있게 한다.
	PlayerController->ClientTravel(LobbyPath + TEXT("?listen"), TRAVEL_Absolute);
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
	ULSSaveSubsystem* SaveSubsystem = GetSaveSubsystem();
	if (!SaveSubsystem)
	{
		UE_LOG(LogLS, Warning, TEXT("[FarmingGameMode] Cannot start signal gauge drain because SaveSubsystem is missing."));
		return;
	}

	float NextPercent = 0.0f;
	if (!LSChipStats::TryResolveNextSignalGaugePercent(
		SaveSubsystem->GetChipEquipmentSlots(),
		SaveSubsystem->GetChipSignalGaugePercent(),
		NextPercent))
	{
		SaveSubsystem->SetChipSignalGaugePercent(0.0f);
		StopSignalGaugeDrain();
		return;
	}

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

	float NextPercent = 0.0f;
	const bool bHasNextChip = LSChipStats::TryResolveNextSignalGaugePercent(
		SaveSubsystem->GetChipEquipmentSlots(),
		SaveSubsystem->GetChipSignalGaugePercent(),
		NextPercent);
	SaveSubsystem->SetChipSignalGaugePercent(NextPercent);

	if (UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(Iterator->Get());
			const ULSRaidInventoryComponent* RaidInventory = PlayerController ? PlayerController->GetRaidInventoryComponent() : nullptr;
			if (!PlayerController || !RaidInventory || !RaidInventory->IsRaidActive())
			{
				continue;
			}

			PlayerController->ClientApplyRaidSignalGaugePercent(NextPercent);
			PlayerController->DropOverflowInventorySlotsToWorld();
		}
	}

	if (!bHasNextChip || NextPercent <= 0.0f)
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
