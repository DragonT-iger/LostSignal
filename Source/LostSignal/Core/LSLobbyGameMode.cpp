#include "Core/LSLobbyGameMode.h"

#include "Blueprint/UserWidget.h"
#include "Core/LSPlayerControllerBase.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/LSRaidInventoryComponent.h"
#include "LostSignal.h"
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
	CreateLobbyBackgroundWidget();
	CreateLobbyMenuWidget();
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
