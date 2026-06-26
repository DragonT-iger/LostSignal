#pragma once

#include "CoreMinimal.h"
#include "Core/LSGameModeBase.h"
#include "Session/LSSessionSubsystem.h"
#include "TimerManager.h"
#include "LSFarmingGameMode.generated.h"

class ALSPlayerControllerBase;
class ULSSaveSubsystem;

UCLASS()
class LOSTSIGNAL_API ALSFarmingGameMode : public ALSGameModeBase
{
	GENERATED_BODY()

public:
	virtual void StartPlay() override;

	// 플레이어 사망 시 — 캐릭터 사망 처리에서 호출
	UFUNCTION(BlueprintCallable, Category="LS/Farming")
	void OnPlayerDied();

	// 탈출 포인트 도달 시 — ExtractionZone 액터가 호출
	UFUNCTION(BlueprintCallable, Category="LS/Farming")
	void OnExtraction();

	// UI 탈주 버튼 or 강제 종료 시 호출
	UFUNCTION(BlueprintCallable, Category="LS/Farming")
	void OnQuit();

	void NotifyRaidResultSaved(ALSPlayerControllerBase* PlayerController);

private:
	void EndRaid(ELSRaidResult Result);
	void BeginRaidResultSave(ELSRaidResult Result);
	bool BuildRaidResultForPlayer(const ALSPlayerControllerBase* PlayerController, ELSRaidResult Result, TArray<FLSSessionItem>& OutInventoryItems, TArray<FLSSessionItem>& OutSafeItems, bool& bOutSaveInventory, bool& bOutSaveSafeStash) const;
	void HandleRaidResultSaveTimeout();
	void TravelToResultLevel();
	void ClearRaidResultSaveWait();

	// 레이드 진입 후 신호 게이지를 시간에 따라 자동 감소시킨다(1분에 10%씩, 0%에서 정지).
	void StartSignalGaugeDrain();
	void TickSignalGaugeDrain();
	void StopSignalGaugeDrain();
	ULSSaveSubsystem* GetSaveSubsystem() const;

	FTimerHandle SignalGaugeDrainTimerHandle;

	UPROPERTY(Transient, VisibleAnywhere, Category="LS/Farming")
	bool bRaidEnded = false;

	// 이번 레이드를 종료시킨 결과 — 탈출(Extracted)은 일단 ResultLevel을 건너뛰고 로비로 복귀
	UPROPERTY(Transient, VisibleAnywhere, Category="LS/Farming")
	ELSRaidResult PendingRaidResult = ELSRaidResult::Extracted;

	UPROPERTY(Transient, VisibleAnywhere, Category="LS/Farming")
	bool bWaitingForRaidResultSave = false;

	UPROPERTY(Transient, VisibleAnywhere, Category="LS/Farming")
	TArray<TObjectPtr<ALSPlayerControllerBase>> PendingRaidResultSaveControllers;

	FTimerHandle RaidResultSaveTimeoutTimerHandle;
};
