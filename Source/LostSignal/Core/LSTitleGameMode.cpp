#include "Core/LSTitleGameMode.h"

#include "Blueprint/UserWidget.h"
#include "Core/LSPlayerControllerBase.h"
#include "Engine/NetConnection.h"
#include "Engine/NetDriver.h"
#include "GameFramework/PlayerController.h"
#include "LostSignal.h"
#include "Session/LSSessionSettings.h"
#include "Session/LSSessionSubsystem.h"
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

void ALSTitleGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	if (!ErrorMessage.IsEmpty())
	{
		return;
	}

	// 타이틀은 접속 지점이 아니다. 방은 로비에서 열고 참가도 로비에서 한다.
	// (PreLogin은 원격 접속에만 불린다 — 로컬 플레이어는 Login으로 바로 들어온다)
	ErrorMessage = LSNetRejectReason::LobbyNotReady;
	UE_LOG(LogLS, Log, TEXT("[Title] Rejected a join from %s. The title screen does not accept connections."), *Address);
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
	// 타이틀은 접속을 받지 않으므로(PreLogin에서 거절) 여기 오는 건 로컬 플레이어뿐이다.
	APlayerController* PlayerController = Cast<APlayerController>(Controller);
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return;
	}

	if (!TitleMenuWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("[Title] TitleMenuWidgetClass is not set on %s. Check BP_TitleGameMode."), *GetNameSafe(this));
		return;
	}

	// GameMode는 서버에만 존재한다. 위젯은 PlayerController가 자기 화면에 만든다.
	if (ALSPlayerControllerBase* LSPlayerController = Cast<ALSPlayerControllerBase>(PlayerController))
	{
		LSPlayerController->ShowTitleMenuWidget(TitleMenuWidgetClass);
		return;
	}

	CreateTitleMenuLocally(PlayerController);
}

void ALSTitleGameMode::CreateTitleMenuLocally(APlayerController* PlayerController)
{
	// PlayerControllerClass가 지정되지 않아 엔진 기본 APlayerController가 온 경우의 폴백.
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
