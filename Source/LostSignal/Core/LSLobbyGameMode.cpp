#include "Core/LSLobbyGameMode.h"

#include "Blueprint/UserWidget.h"
#include "Core/LSPlayerControllerBase.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/LSRaidInventoryComponent.h"
#include "LostSignal.h"
#include "Session/LSSaveSubsystem.h"
#include "Session/LSSessionSettings.h"
#include "Session/LSSessionSubsystem.h"
#include "UI/LSBackgroundBlurWidget.h"
#include "UI/LSUILayer.h"
#include "UI/Lobby/LSLobbyMenuWidget.h"

namespace
{
constexpr float RaidEntryDataTimeoutSeconds = 10.0f;
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
	CreateLobbyBackgroundWidget();
	CreateLobbyMenuWidget();
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

void ALSLobbyGameMode::CreateLobbyBackgroundWidget()
{
	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PlayerController)
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] Cannot create lobby background because PlayerController is missing."));
		return;
	}

	if (!BackgroundBlurWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] BackgroundBlurWidgetClass is not set on %s. Check BP_LobbyGameMode."), *GetNameSafe(this));
		return;
	}

	BackgroundBlurWidgetInstance = CreateWidget<ULSBackgroundBlurWidget>(PlayerController, BackgroundBlurWidgetClass);
	if (!BackgroundBlurWidgetInstance)
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] Failed to create lobby background widget on %s."), *GetNameSafe(this));
		return;
	}

	// 메뉴 뒤에 상시 깔리는 배경. 입력은 메뉴가 받도록 HitTestInvisible로 둔다.
	BackgroundBlurWidgetInstance->AddToViewport(LSUILayer::LobbyBackground);
	BackgroundBlurWidgetInstance->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void ALSLobbyGameMode::CreateLobbyMenuWidget()
{
	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PlayerController)
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] Cannot create lobby menu because PlayerController is missing."));
		return;
	}

	if (!LobbyMenuWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] LobbyMenuWidgetClass is not set on %s. Check BP_LobbyGameMode."), *GetNameSafe(this));
		return;
	}

	LobbyMenuWidgetInstance = CreateWidget<ULSLobbyMenuWidget>(PlayerController, LobbyMenuWidgetClass);
	if (!LobbyMenuWidgetInstance)
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] Failed to create lobby menu widget on %s."), *GetNameSafe(this));
		return;
	}

	LobbyMenuWidgetInstance->AddToViewport(LSUILayer::LobbyMenu);

	PlayerController->bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	// 로비 메뉴에 포커스를 줘서 TAB 등 키 입력이 위젯으로 전달되게 한다.
	InputMode.SetWidgetToFocus(LobbyMenuWidgetInstance->TakeWidget());
	PlayerController->SetInputMode(InputMode);
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

	TArray<FLSSessionItem> Loadout;
	TArray<FLSSessionItem> SafeItems;
	TArray<FLSSessionItem> Equipment;
	bool bHasLegacySessionLoadout = false;

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (ALSPlayerControllerBase* PlayerController = Cast<ALSPlayerControllerBase>(It->Get()))
		{
			const TArray<FLSSessionItem>& PlayerLoadout = PlayerController->GetSubmittedRaidLoadout();
			const TArray<FLSSessionItem>& PlayerSafeItems = PlayerController->GetSubmittedRaidSafeItems();
			const TArray<FLSSessionItem>& PlayerEquipment = PlayerController->GetSubmittedRaidEquipment();
			if (!bHasLegacySessionLoadout)
			{
				Loadout = PlayerLoadout;
				SafeItems = PlayerSafeItems;
				Equipment = PlayerEquipment;
				bHasLegacySessionLoadout = true;
			}

			// ServerTravel 이후 각 PC가 자신의 데이터를 꺼낼 수 있도록 순서대로 큐에 저장
			SessionSub->EnqueuePendingRaidEntry(PlayerLoadout, PlayerSafeItems, PlayerEquipment);

			if (ULSRaidInventoryComponent* RaidInventory = PlayerController->GetRaidInventoryComponent())
			{
				RaidInventory->StartRaidInventory(PlayerLoadout, PlayerSafeItems, PlayerEquipment);
			}
			PlayerController->ClientStartRaidSession(PlayerLoadout, PlayerSafeItems, PlayerEquipment);
		}
	}

	SessionSub->StartRaid(Loadout);
	SessionSub->MirrorRaidSessionState(Loadout, SafeItems, Equipment);
	ClearRaidEntryDataWait();
	bRaidStartRequested = true;

	if (!World->ServerTravel(RaidLevelPath))
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] Failed to server travel to raid level: %s"), *RaidLevelPath);
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
