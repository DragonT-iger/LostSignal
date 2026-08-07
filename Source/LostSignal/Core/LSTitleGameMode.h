#pragma once

#include "CoreMinimal.h"
#include "Core/LSGameModeBase.h"
#include "TimerManager.h"
#include "LSTitleGameMode.generated.h"

class ULSTitleMenuWidget;

// 타이틀 레벨 전용 GameMode. WBP_TitleMenu를 뷰포트에 올리고 UI 입력 모드로 전환한다.
UCLASS()
class LOSTSIGNAL_API ALSTitleGameMode : public ALSGameModeBase
{
	GENERATED_BODY()

public:
	ALSTitleGameMode();

	// 타이틀은 접속을 받지 않는다. 방은 로비에서 열고 참가도 로비에서 한다.
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	// seamless travel로 들어온 플레이어는 PostLogin을 타지 않으므로 여기서도 타이틀 메뉴를 띄운다.
	virtual void HandleSeamlessTravelPlayer(AController*& C) override;

	void RequestOpenLobbyLevel();

protected:
	// BP(BP_TitleGameMode)에서 WBP_TitleMenu를 매핑한다.
	UPROPERTY(EditDefaultsOnly, Category="LS/Title")
	TSubclassOf<ULSTitleMenuWidget> TitleMenuWidgetClass;


private:
	// 해당 플레이어 화면에 타이틀 메뉴를 띄운다. 위젯 생성은 각 PlayerController가 한다
	// (GameMode는 서버에만 있어서 여기서 만들면 호스트만 보인다).
	void ShowTitleMenuFor(AController* Controller);
	// PlayerControllerClass가 ALSPlayerControllerBase가 아닐 때의 로컬 전용 폴백.
	void CreateTitleMenuLocally(APlayerController* PlayerController);
	bool HasPendingPlayerConnections() const;
	void TryOpenLobbyLevel();
	void HandlePendingPlayerConnectionTimeout();

	bool bLobbyTravelRequested = false;
	FTimerHandle PendingPlayerConnectionPollTimerHandle;
	FTimerHandle PendingPlayerConnectionTimeoutTimerHandle;
};
