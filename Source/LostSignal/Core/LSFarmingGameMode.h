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

	// 신호 게이지가 다음 10% 단계(= 칩 1칸)로 떨어지기까지 남은 초. 드레인 타이머가 멈춰 있으면 음수.
	// HUD 생존 링이 자체 추정 대신 실제 잔여시간을 읽도록 노출한다(서버 권한에서만 유효).
	float GetSignalGaugeDrainRemainingSeconds() const;

	// 드레인 한 주기 길이(초). 신호 링 카운트다운의 분모로 쓰여 60초 값을 단일 출처화한다.
	float GetSignalGaugeDrainInterval() const;

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

	// 사망 연출(몽타주 등)을 보여준 뒤 레이드 종료를 시작하기까지의 대기 시간(초)
	UPROPERTY(EditDefaultsOnly, Category="LS/Farming", meta=(ClampMin="0.0"))
	float DeathRaidEndDelaySeconds = 3.0f;

	FTimerHandle DeathRaidEndTimerHandle;

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
