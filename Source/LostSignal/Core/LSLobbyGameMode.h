#pragma once

#include "CoreMinimal.h"
#include "Core/LSGameModeBase.h"
#include "TimerManager.h"
#include "LSLobbyGameMode.generated.h"

class ALSPlayerControllerBase;

UCLASS()
class LOSTSIGNAL_API ALSLobbyGameMode : public ALSGameModeBase
{
	GENERATED_BODY()

public:
	// 로비에서 레이드 시작 (로드아웃은 인벤토리 시스템 구현 후 연결)
	UFUNCTION(BlueprintCallable, Category="LS/Lobby")
	void StartRaid();

	void NotifyRaidEntryDataSubmitted(ALSPlayerControllerBase* PlayerController);

private:
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
