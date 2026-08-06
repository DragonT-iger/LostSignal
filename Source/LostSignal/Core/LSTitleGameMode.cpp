#include "Core/LSTitleGameMode.h"

#include "Blueprint/UserWidget.h"
#include "Engine/NetConnection.h"
#include "Engine/NetDriver.h"
#include "GameFramework/PlayerController.h"
#include "LostSignal.h"
#include "Session/LSSessionSettings.h"
#include "UI/Title/LSTitleMenuWidget.h"

namespace
{
constexpr float LSTitlePendingConnectionPollIntervalSeconds = 0.1f;
constexpr float LSTitlePendingConnectionTimeoutSeconds = 10.0f;
}

ALSTitleGameMode::ALSTitleGameMode()
{
	// 타이틀은 조작할 폰이 없다. 기본 폰 스폰을 막는다.
	DefaultPawnClass = nullptr;
}

void ALSTitleGameMode::BeginPlay()
{
	Super::BeginPlay();
	CreateTitleMenuWidget();
}

void ALSTitleGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	UE_LOG(LogLS, Log, TEXT("[Title] Player login completed. Player=%s ConnectedPlayers=%d"),
		*GetNameSafe(NewPlayer), GetNumPlayers());

	if (bLobbyTravelRequested)
	{
		TryOpenLobbyLevel();
	}
}

void ALSTitleGameMode::RequestOpenLobbyLevel()
{
	if (bLobbyTravelRequested)
	{
		return;
	}

	bLobbyTravelRequested = true;
	GetWorldTimerManager().SetTimer(
		PendingPlayerConnectionTimeoutTimerHandle,
		this,
		&ALSTitleGameMode::HandlePendingPlayerConnectionTimeout,
		LSTitlePendingConnectionTimeoutSeconds,
		false);
	TryOpenLobbyLevel();
}

bool ALSTitleGameMode::HasPendingPlayerConnections() const
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

void ALSTitleGameMode::TryOpenLobbyLevel()
{
	if (!bLobbyTravelRequested)
	{
		return;
	}

	if (HasPendingPlayerConnections())
	{
		if (!GetWorldTimerManager().IsTimerActive(PendingPlayerConnectionPollTimerHandle))
		{
			UE_LOG(LogLS, Log, TEXT("[Title] Lobby travel is waiting for pending player login."));
			GetWorldTimerManager().SetTimer(
				PendingPlayerConnectionPollTimerHandle,
				this,
				&ALSTitleGameMode::TryOpenLobbyLevel,
				LSTitlePendingConnectionPollIntervalSeconds,
				true);
		}
		return;
	}

	const ULSSessionSettings* Settings = GetDefault<ULSSessionSettings>();
	const FString LobbyLevelPath = Settings ? Settings->LobbyLevel.ToSoftObjectPath().GetLongPackageName() : FString();
	if (LobbyLevelPath.IsEmpty() || !GetWorld()->ServerTravel(LobbyLevelPath))
	{
		UE_LOG(LogLS, Warning, TEXT("[Title] Failed to server travel to lobby level: %s"), *LobbyLevelPath);
		bLobbyTravelRequested = false;
	}

	GetWorldTimerManager().ClearTimer(PendingPlayerConnectionPollTimerHandle);
	GetWorldTimerManager().ClearTimer(PendingPlayerConnectionTimeoutTimerHandle);
}

void ALSTitleGameMode::HandlePendingPlayerConnectionTimeout()
{
	if (!bLobbyTravelRequested)
	{
		return;
	}

	UE_LOG(LogLS, Warning, TEXT("[Title] Timed out waiting for a pending player login. Lobby travel cancelled."));
	bLobbyTravelRequested = false;
	GetWorldTimerManager().ClearTimer(PendingPlayerConnectionPollTimerHandle);
}

void ALSTitleGameMode::CreateTitleMenuWidget()
{
	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PlayerController)
	{
		UE_LOG(LogLS, Warning, TEXT("[Title] Cannot create title menu because PlayerController is missing."));
		return;
	}

	if (!TitleMenuWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("[Title] TitleMenuWidgetClass is not set on %s. Check BP_TitleGameMode."), *GetNameSafe(this));
		return;
	}

	TitleMenuWidgetInstance = CreateWidget<ULSTitleMenuWidget>(PlayerController, TitleMenuWidgetClass);
	if (!TitleMenuWidgetInstance)
	{
		UE_LOG(LogLS, Warning, TEXT("[Title] Failed to create title menu widget on %s."), *GetNameSafe(this));
		return;
	}

	TitleMenuWidgetInstance->AddToViewport();

	PlayerController->bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
}
