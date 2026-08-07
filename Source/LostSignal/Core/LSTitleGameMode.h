#pragma once

#include "CoreMinimal.h"
#include "Core/LSGameModeBase.h"
#include "TimerManager.h"
#include "LSTitleGameMode.generated.h"

class ULSLobbyMenuWidget;
class ULSTitleMenuWidget;

// 타이틀 레벨 전용 GameMode. WBP_TitleMenu를 뷰포트에 올리고 UI 입력 모드로 전환한다.
UCLASS()
class LOSTSIGNAL_API ALSTitleGameMode : public ALSGameModeBase
{
	GENERATED_BODY()

public:
	ALSTitleGameMode();

	virtual void PostLogin(APlayerController* NewPlayer) override;
	// seamless travel로 들어온 플레이어는 PostLogin을 타지 않으므로 여기서도 타이틀 메뉴를 띄운다.
	virtual void HandleSeamlessTravelPlayer(AController*& C) override;

	void RequestOpenLobbyLevel();

protected:
	// BP(BP_TitleGameMode)에서 WBP_TitleMenu를 매핑한다.
	UPROPERTY(EditDefaultsOnly, Category="LS/Title")
	TSubclassOf<ULSTitleMenuWidget> TitleMenuWidgetClass;

	// 손님(원격 접속자)에게 타이틀 메뉴 대신 보여줄 로비 화면. BP(BP_TitleGameMode)에서 WBP_LobbyMenu를 매핑한다.
	// 접속하면 서버의 현재 맵을 무조건 로드하므로 호스트가 타이틀에 있으면 손님도 타이틀 레벨에 들어온다.
	// 로비 UI는 폰·월드 의존이 없고 표시 데이터가 전부 클라 로컬 세이브라 어느 맵에서도 성립한다.
	UPROPERTY(EditDefaultsOnly, Category="LS/Title")
	TSubclassOf<ULSLobbyMenuWidget> GuestLobbyMenuWidgetClass;

private:
	// 해당 플레이어 화면에 타이틀 메뉴를 띄운다. 위젯 생성은 각 PlayerController가 한다
	// (GameMode는 서버에만 있어서 여기서 만들면 호스트만 보인다).
	void ShowTitleMenuFor(AController* Controller);
	// 손님에게 자기 로비 화면을 띄운다. 타이틀 메뉴를 주면 Continue 같은 호스트 전용 버튼이
	// 눌리기만 하고 동작하지 않는다(클라에는 GameMode가 없다).
	void ShowGuestLobbyMenuFor(APlayerController* PlayerController);
	// PlayerControllerClass가 ALSPlayerControllerBase가 아닐 때의 로컬 전용 폴백.
	void CreateTitleMenuLocally(APlayerController* PlayerController);
	bool HasPendingPlayerConnections() const;
	void TryOpenLobbyLevel();
	void HandlePendingPlayerConnectionTimeout();

	bool bLobbyTravelRequested = false;
	FTimerHandle PendingPlayerConnectionPollTimerHandle;
	FTimerHandle PendingPlayerConnectionTimeoutTimerHandle;
};
