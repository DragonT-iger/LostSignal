#pragma once

#include "CoreMinimal.h"
#include "Core/LSGameModeBase.h"
#include "TimerManager.h"
#include "LSLobbyGameMode.generated.h"

class ALSPlayerControllerBase;
class ULSLobbyMenuWidget;

UCLASS()
class LOSTSIGNAL_API ALSLobbyGameMode : public ALSGameModeBase
{
	GENERATED_BODY()

public:
	ALSLobbyGameMode();

	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	// seamless travel로 들어온 플레이어는 PostLogin을 타지 않으므로 여기서도 로비 메뉴를 띄운다.
	virtual void HandleSeamlessTravelPlayer(AController*& C) override;
	virtual void Logout(AController* Exiting) override;

	// 로비에서 레이드 시작 (로드아웃은 인벤토리 시스템 구현 후 연결)
	UFUNCTION(BlueprintCallable, Category="LS/Lobby")
	void StartRaid();

	// 디버그: 정식 레이드 진입 시퀀스(로드아웃 제출·세션/세이브 셋업)를 그대로 실행하되 목적지만
	// Settings->TestRaidLevel 로 바꿔 진입한다. 레이드 환경이 100% 동일(결과 저장 포함).
	// (프로토콜 디버그 패널의 "테스트 맵 가기" 버튼이 호출 — 로비에서만 동작)
	void StartRaidToTestLevel();

	void NotifyRaidEntryDataSubmitted(ALSPlayerControllerBase* PlayerController);

protected:
	// BP(BP_LobbyGameMode)에서 WBP_Lobby를 매핑한다.
	UPROPERTY(EditDefaultsOnly, Category="LS/Lobby")
	TSubclassOf<ULSLobbyMenuWidget> LobbyMenuWidgetClass;

private:
	// 로비 진입 시 신호 게이지를 가득으로 되돌린다(레이드 복구 대기 중이면 보존). 레이드 전용 신호 감소가
	// 로비 적재 프로토콜 용량을 축소해 인벤토리 overflow를 만드는 문제를 막는다.
	void RestoreLobbySignalGauge();

	// 해당 플레이어 화면에 로비 메뉴를 띄운다. 위젯 생성은 각 PlayerController가 한다
	// (GameMode는 서버에만 있어서 여기서 만들면 호스트만 보인다).
	void ShowLobbyMenuFor(AController* Controller);

	UPROPERTY(Transient, VisibleAnywhere, Category="LS/Lobby")
	bool bRaidStartRequested = false;

	// 설정 시 이번 진입의 목적지를 FarmingLevel 대신 이 레벨로 바꾼다(디버그 테스트 맵). TryStart 에서 1회 소비.
	UPROPERTY(Transient)
	TSoftObjectPtr<UWorld> RaidLevelOverride;

	UPROPERTY(Transient, VisibleAnywhere, Category="LS/Lobby")
	bool bWaitingForRaidEntryData = false;

	// ServerTravel을 이미 걸었다. seamless travel 중에는 원격 PC가 하나씩 Logout되는데, 그때마다
	// Logout 핸들러가 입장 데이터를 다시 수집하고 ServerTravel을 또 거는 것을 막는다.
	UPROPERTY(Transient, VisibleAnywhere, Category="LS/Lobby")
	bool bRaidTravelStarted = false;

	bool RequestRaidEntryDataFromPlayers();
	bool AreRaidEntryDataReady() const;
	bool HasPendingPlayerConnections() const;
	void TryBeginRaidEntryDataCollection();
	void PollPendingPlayerConnections();
	void TryStartRaidWithSubmittedData();
	void HandleRaidEntryDataTimeout();
	void ClearRaidEntryDataWait();

	FTimerHandle RaidEntryDataTimeoutTimerHandle;
	FTimerHandle PendingPlayerConnectionPollTimerHandle;
};
