#pragma once

#include "CoreMinimal.h"
#include "Core/LSGameModeBase.h"
#include "TimerManager.h"
#include "LSLobbyGameMode.generated.h"

class ALSPlayerControllerBase;
class ULSLobbyMenuWidget;
class ULSBackgroundBlurWidget;

UCLASS()
class LOSTSIGNAL_API ALSLobbyGameMode : public ALSGameModeBase
{
	GENERATED_BODY()

public:
	ALSLobbyGameMode();

	virtual void BeginPlay() override;

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

	// BP(BP_LobbyGameMode)에서 모달용과 동일한 WBP_BackgroundBlur를 매핑한다. 로비 메뉴 뒤에 상시 깔린다.
	UPROPERTY(EditDefaultsOnly, Category="LS/Lobby")
	TSubclassOf<ULSBackgroundBlurWidget> BackgroundBlurWidgetClass;

private:
	// 로비 진입 시 신호 게이지를 가득으로 되돌린다(레이드 복구 대기 중이면 보존). 레이드 전용 신호 감소가
	// 로비 적재 프로토콜 용량을 축소해 인벤토리 overflow를 만드는 문제를 막는다.
	void RestoreLobbySignalGauge();

	// 레벨 진입 즉시 로비 배경 블러와 메뉴 UI를 뷰포트에 올리고 UI 입력 모드로 전환한다.
	void CreateLobbyBackgroundWidget();
	void CreateLobbyMenuWidget();

	UPROPERTY(Transient)
	TObjectPtr<ULSLobbyMenuWidget> LobbyMenuWidgetInstance;

	UPROPERTY(Transient)
	TObjectPtr<ULSBackgroundBlurWidget> BackgroundBlurWidgetInstance;

	UPROPERTY(Transient, VisibleAnywhere, Category="LS/Lobby")
	bool bRaidStartRequested = false;

	// 설정 시 이번 진입의 목적지를 FarmingLevel 대신 이 레벨로 바꾼다(디버그 테스트 맵). TryStart 에서 1회 소비.
	UPROPERTY(Transient)
	TSoftObjectPtr<UWorld> RaidLevelOverride;

	UPROPERTY(Transient, VisibleAnywhere, Category="LS/Lobby")
	bool bWaitingForRaidEntryData = false;

	bool RequestRaidEntryDataFromPlayers();
	bool AreRaidEntryDataReady() const;
	void TryStartRaidWithSubmittedData();
	void HandleRaidEntryDataTimeout();
	void ClearRaidEntryDataWait();

	FTimerHandle RaidEntryDataTimeoutTimerHandle;
};
