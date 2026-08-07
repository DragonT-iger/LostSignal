#include "Core/LSLobbyGameMode.h"

#include "Blueprint/UserWidget.h"
#include "Core/LSPlayerControllerBase.h"
#include "Engine/NetConnection.h"
#include "Engine/NetDriver.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/LSRaidInventoryComponent.h"
#include "LostSignal.h"
#include "Session/LSSaveSubsystem.h"
#include "Session/LSSessionSettings.h"
#include "Session/LSSessionSubsystem.h"
#include "UI/Lobby/LSLobbyMenuWidget.h"

namespace
{
constexpr float LSLobbyRaidEntryDataTimeoutSeconds = 10.0f;
constexpr float LSLobbyPendingConnectionPollIntervalSeconds = 0.1f;
}

ALSLobbyGameMode::ALSLobbyGameMode()
{
	// 로비는 조작할 폰이 없다. 기본 폰 스폰을 막는다.
	DefaultPawnClass = nullptr;
}

void ALSLobbyGameMode::BeginPlay()
{
	Super::BeginPlay();
	RestoreLobbySignalGauge();
}

void ALSLobbyGameMode::HandleSeamlessTravelPlayer(AController*& C)
{
	Super::HandleSeamlessTravelPlayer(C);

	// seamless travel로 들어온 플레이어는 PostLogin을 타지 않는다. 로비 메뉴는 여기서 띄운다.
	ShowLobbyMenuFor(C);
}

void ALSLobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	UE_LOG(LogLS, Log, TEXT("[Lobby] Player login completed. Player=%s ConnectedPlayers=%d"),
		*GetNameSafe(NewPlayer), GetNumPlayers());

	ShowLobbyMenuFor(NewPlayer);

	if (!bRaidStartRequested || bRaidTravelStarted)
	{
		return;
	}

	if (bWaitingForRaidEntryData)
	{
		if (ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(NewPlayer))
		{
			PlayerController->RequestRaidEntryDataForRaidStart();
		}
		return;
	}

	TryBeginRaidEntryDataCollection();
}

void ALSLobbyGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
	UE_LOG(LogLS, Log, TEXT("[Lobby] Player logged out. Player=%s ConnectedPlayers=%d"),
		*GetNameSafe(Exiting), GetNumPlayers());

	// ServerTravel이 이미 걸렸으면 여기 들어오는 Logout은 seamless travel이 원격 PC를 정리하는 과정이다.
	// 그때 입장 데이터를 다시 수집하면 큐가 중복 적재되고 ServerTravel이 여러 번 걸린다.
	if (!bRaidStartRequested || bRaidTravelStarted)
	{
		return;
	}

	if (bWaitingForRaidEntryData)
	{
		TryStartRaidWithSubmittedData();
	}
	else
	{
		TryBeginRaidEntryDataCollection();
	}
}

void ALSLobbyGameMode::RestoreLobbySignalGauge()
{
	// 신호 게이지는 레이드 전용 감소 메커니즘이다(레이드 중 시간에 따라 줄며, 줄면 칩이 비활성→적재 프로토콜 용량 축소).
	// 값은 매 틱 세이브에 저장되므로, 레이드가 비정상 종료(PIE 강제 종료/크래시)돼 TravelToResultLevel의 1.0 리셋을
	// 못 타면 줄어든 값이 세이브에 남는다. 그 상태로 로비에 오면 로비 인벤토리 최대 슬롯 수가 실제 아이템 수보다
	// 작아져, 초과 아이템이 화면에 안 보이는 overflow가 되고 칩 해제 용량 판정이 계속 막힌다.
	// 로비(=레이드 아님)에서는 신호를 항상 가득으로 되돌려 로비 용량이 신호로 축소되지 않게 한다.
	// 단, 레이드 복구 대기(bRaidSaveActive) 중이면 재개용 값을 보존한다.
	UGameInstance* GameInstance = GetGameInstance();
	ULSSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
	if (!SaveSubsystem)
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] Cannot restore signal gauge because SaveSubsystem is missing on %s."), *GetNameSafe(this));
		return;
	}

	if (SaveSubsystem->IsRaidSaveActive())
	{
		return;
	}

	SaveSubsystem->SetChipSignalGaugePercent(1.0f);
}

void ALSLobbyGameMode::ShowLobbyMenuFor(AController* Controller)
{
	if (!LobbyMenuWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] LobbyMenuWidgetClass is not set on %s. Check BP_LobbyGameMode."), *GetNameSafe(this));
		return;
	}

	// GameMode는 서버에만 존재하므로 위젯을 여기서 직접 만들면 호스트 화면에만 뜬다.
	// 각 PlayerController가 자기 화면에 만들도록 위젯 클래스를 넘긴다(원격은 Client RPC).
	ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(Controller);
	if (!PlayerController)
	{
		// 로비는 참가자가 들어오는 지점이라 폴백을 두지 않는다. PlayerControllerClass가 잘못됐다는 뜻이다.
		UE_LOG(LogLS, Warning, TEXT("[Lobby] Cannot show the lobby menu for %s. PlayerControllerClass must derive from ALSPlayerControllerBase. Check BP_LobbyGameMode."),
			*GetNameSafe(Controller));
		return;
	}

	PlayerController->ShowLobbyMenuWidget(LobbyMenuWidgetClass);
}

void ALSLobbyGameMode::StartRaid()
{
	if (bRaidStartRequested || bRaidTravelStarted)
	{
		return;
	}

	bRaidStartRequested = true;
	GetWorldTimerManager().SetTimer(
		RaidEntryDataTimeoutTimerHandle,
		this,
		&ALSLobbyGameMode::HandleRaidEntryDataTimeout,
		LSLobbyRaidEntryDataTimeoutSeconds,
		false);

	TryBeginRaidEntryDataCollection();
}

void ALSLobbyGameMode::StartRaidToTestLevel()
{
	const ULSSessionSettings* Settings = GetDefault<ULSSessionSettings>();
	if (!Settings || Settings->TestRaidLevel.IsNull())
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] TestRaidLevel is not set. Check Project Settings > LS Session Settings."));
		return;
	}

	// 정식 진입 경로(StartRaid)를 그대로 타되 목적지만 테스트 레벨로 덮어쓴다(1회).
	RaidLevelOverride = Settings->TestRaidLevel;
	StartRaid();
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

bool ALSLobbyGameMode::HasPendingPlayerConnections() const
{
	const UWorld* World = GetWorld();
	const UNetDriver* NetDriver = World ? World->GetNetDriver() : nullptr;
	if (!NetDriver)
	{
		return false;
	}

	for (const UNetConnection* Connection : NetDriver->ClientConnections)
	{
		if (Connection
			&& !Connection->IsClosingOrClosed()
			&& Connection->ClientLoginState != EClientLoginState::ReceivedJoin)
		{
			return true;
		}
	}

	return false;
}

void ALSLobbyGameMode::TryBeginRaidEntryDataCollection()
{
	if (!bRaidStartRequested || bWaitingForRaidEntryData || bRaidTravelStarted)
	{
		return;
	}

	if (HasPendingPlayerConnections())
	{
		if (!GetWorldTimerManager().IsTimerActive(PendingPlayerConnectionPollTimerHandle))
		{
			UE_LOG(LogLS, Log, TEXT("[Lobby] Raid start is waiting for pending player login."));
			GetWorldTimerManager().SetTimer(
				PendingPlayerConnectionPollTimerHandle,
				this,
				&ALSLobbyGameMode::PollPendingPlayerConnections,
				LSLobbyPendingConnectionPollIntervalSeconds,
				true);
		}
		return;
	}

	GetWorldTimerManager().ClearTimer(PendingPlayerConnectionPollTimerHandle);
	bWaitingForRaidEntryData = true;
	if (!RequestRaidEntryDataFromPlayers())
	{
		ClearRaidEntryDataWait();
		return;
	}

	TryStartRaidWithSubmittedData();
}

void ALSLobbyGameMode::PollPendingPlayerConnections()
{
	if (!bRaidStartRequested)
	{
		GetWorldTimerManager().ClearTimer(PendingPlayerConnectionPollTimerHandle);
		return;
	}

	if (!HasPendingPlayerConnections())
	{
		UE_LOG(LogLS, Log, TEXT("[Lobby] Pending player login completed. Collecting raid entry data from %d players."),
			GetNumPlayers());
		TryBeginRaidEntryDataCollection();
	}
}

void ALSLobbyGameMode::TryStartRaidWithSubmittedData()
{
	if (!bRaidStartRequested || !bWaitingForRaidEntryData || bRaidTravelStarted || !AreRaidEntryDataReady())
	{
		return;
	}

	const ULSSessionSettings* Settings = GetDefault<ULSSessionSettings>();
	// 디버그 테스트 맵 진입이면 목적지만 TestRaidLevel 로 바꾸고, 나머지 세션 셋업은 동일하게 진행한다.
	const TSoftObjectPtr<UWorld> DestinationLevel = !RaidLevelOverride.IsNull()
		? RaidLevelOverride
		: (Settings ? Settings->FarmingLevel : TSoftObjectPtr<UWorld>());
	RaidLevelOverride.Reset();
	if (DestinationLevel.IsNull())
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] Raid destination level is not set. Check Project Settings > LS Session Settings (FarmingLevel/TestRaidLevel)."));
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

	const FString RaidLevelPath = DestinationLevel.ToSoftObjectPath().GetLongPackageName();
	if (RaidLevelPath.IsEmpty())
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] Cannot start raid because destination level path is invalid."));
		ClearRaidEntryDataWait();
		return;
	}

	// 플레이어별 payload의 원본은 각 ALSPlayerState다. seamless travel에서 컨트롤러는 전부 새로 스폰되지만
	// PlayerState는 CopyProperties로 값이 건너가므로, 파밍 레벨에서 각자 자기 것을 그대로 복원한다.
	// (예전의 SessionSubsystem 순서 큐는 접속 순서에 의존해 3인에서 인벤토리를 섞었다)
	int32 StartedPlayerCount = 0;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(It->Get());
		if (!PlayerController)
		{
			continue;
		}

		const TArray<FLSSessionItem>& PlayerLoadout = PlayerController->GetSubmittedRaidLoadout();
		const TArray<FLSSessionItem>& PlayerSafeItems = PlayerController->GetSubmittedRaidSafeItems();
		const TArray<FLSSessionItem>& PlayerEquipment = PlayerController->GetSubmittedRaidEquipment();

		if (ULSRaidInventoryComponent* RaidInventory = PlayerController->GetRaidInventoryComponent())
		{
			RaidInventory->StartRaidInventory(PlayerLoadout, PlayerSafeItems, PlayerEquipment);
		}
		PlayerController->ClientStartRaidSession(PlayerLoadout, PlayerSafeItems, PlayerEquipment);
		++StartedPlayerCount;
	}

	// 레이드 복구 저장(BeginRaidSave)은 각자 ClientStartRaidSession에서 자기 로드아웃으로 시작한다.
	// SessionSubsystem의 전역 세션 상태는 3인에서 정의상 누군가에게 틀린 값이라 더 이상 쓰지 않는다.
	ClearRaidEntryDataWait();
	bRaidTravelStarted = true;

	if (!World->ServerTravel(RaidLevelPath))
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] Failed to server travel to raid level: %s"), *RaidLevelPath);
		bRaidTravelStarted = false;
		return;
	}

	UE_LOG(LogLS, Log, TEXT("[Lobby] Raid start travel issued for %d players. Level=%s"), StartedPlayerCount, *RaidLevelPath);
}

void ALSLobbyGameMode::HandleRaidEntryDataTimeout()
{
	if (!bRaidStartRequested)
	{
		return;
	}

	if (!bWaitingForRaidEntryData)
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] Timed out waiting for a pending player login. Raid start cancelled."));
	}
	else if (UWorld* World = GetWorld())
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
	GetWorldTimerManager().ClearTimer(PendingPlayerConnectionPollTimerHandle);
}
