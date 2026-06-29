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

	void NotifyRaidEntryDataSubmitted(ALSPlayerControllerBase* PlayerController);

protected:
	// BP(BP_LobbyGameMode)에서 WBP_Lobby를 매핑한다.
	UPROPERTY(EditDefaultsOnly, Category="LS/Lobby")
	TSubclassOf<ULSLobbyMenuWidget> LobbyMenuWidgetClass;

	// BP(BP_LobbyGameMode)에서 모달용과 동일한 WBP_BackgroundBlur를 매핑한다. 로비 메뉴 뒤에 상시 깔린다.
	UPROPERTY(EditDefaultsOnly, Category="LS/Lobby")
	TSubclassOf<ULSBackgroundBlurWidget> BackgroundBlurWidgetClass;

private:
	// 레벨 진입 즉시 로비 배경 블러와 메뉴 UI를 뷰포트에 올리고 UI 입력 모드로 전환한다.
	void CreateLobbyBackgroundWidget();
	void CreateLobbyMenuWidget();

	UPROPERTY(Transient)
	TObjectPtr<ULSLobbyMenuWidget> LobbyMenuWidgetInstance;

	UPROPERTY(Transient)
	TObjectPtr<ULSBackgroundBlurWidget> BackgroundBlurWidgetInstance;

	UPROPERTY(Transient, VisibleAnywhere, Category="LS/Lobby")
	bool bRaidStartRequested = false;

	UPROPERTY(Transient, VisibleAnywhere, Category="LS/Lobby")
	bool bWaitingForRaidEntryData = false;

	bool RequestRaidEntryDataFromPlayers();
	bool AreRaidEntryDataReady() const;
	void TryStartRaidWithSubmittedData();
	void HandleRaidEntryDataTimeout();
	void ClearRaidEntryDataWait();

	FTimerHandle RaidEntryDataTimeoutTimerHandle;
};
