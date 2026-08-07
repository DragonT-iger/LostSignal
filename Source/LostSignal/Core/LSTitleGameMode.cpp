#include "Core/LSTitleGameMode.h"

#include "Blueprint/UserWidget.h"
#include "Core/LSPlayerControllerBase.h"
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

void ALSTitleGameMode::HandleSeamlessTravelPlayer(AController*& C)
{
	Super::HandleSeamlessTravelPlayer(C);

	// seamless travel로 들어온 플레이어는 PostLogin을 타지 않는다. 타이틀 메뉴는 여기서 띄운다.
	ShowTitleMenuFor(C);
}

void ALSTitleGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	UE_LOG(LogLS, Log, TEXT("[Title] Player login completed. Player=%s ConnectedPlayers=%d"),
		*GetNameSafe(NewPlayer), GetNumPlayers());

	ShowTitleMenuFor(NewPlayer);

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

void ALSTitleGameMode::ShowTitleMenuFor(AController* Controller)
{
	if (!TitleMenuWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("[Title] TitleMenuWidgetClass is not set on %s. Check BP_TitleGameMode."), *GetNameSafe(this));
		return;
	}

	// GameMode는 서버에만 존재한다. 위젯은 각 PlayerController가 자기 화면에 만든다(원격은 Client RPC).
	if (ALSPlayerControllerBase* LSPlayerController = Cast<ALSPlayerControllerBase>(Controller))
	{
		LSPlayerController->ShowTitleMenuWidget(TitleMenuWidgetClass);
		return;
	}

	// 타이틀 레벨은 PlayerControllerClass를 지정하지 않아 엔진 기본 APlayerController가 온다.
	// 그 경우엔 Client RPC 경로가 없으므로 로컬 컨트롤러에 한해 여기서 직접 만든다.
	// (타이틀은 접속 지점이 아니라 원격 참가자가 올 일이 없다 — 참가는 로비에서 한다)
	APlayerController* PlayerController = Cast<APlayerController>(Controller);
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		UE_LOG(LogLS, Warning, TEXT("[Title] Cannot show the title menu for %s. Set PlayerControllerClass to ALSPlayerControllerBase on BP_TitleGameMode to support remote players."),
			*GetNameSafe(Controller));
		return;
	}

	ULSTitleMenuWidget* TitleMenuWidget = CreateWidget<ULSTitleMenuWidget>(PlayerController, TitleMenuWidgetClass);
	if (!TitleMenuWidget)
	{
		UE_LOG(LogLS, Warning, TEXT("[Title] Failed to create title menu widget on %s."), *GetNameSafe(this));
		return;
	}

	TitleMenuWidget->AddToViewport();

	PlayerController->bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
}
