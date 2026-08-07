#pragma once

#include "CoreMinimal.h"
#include "Core/LSGameModeBase.h"
#include "Session/LSSessionSubsystem.h"
#include "TimerManager.h"
#include "UObject/SoftObjectPtr.h"
#include "LSFarmingGameMode.generated.h"

class ALSPlayerControllerBase;
class ULSSaveSubsystem;

UCLASS()
class LOSTSIGNAL_API ALSFarmingGameMode : public ALSGameModeBase
{
	GENERATED_BODY()

public:
	virtual void StartPlay() override;
	// 레이드 중 난입을 거절한다. 뒤늦게 들어온 플레이어는 로비에서 입장 payload를 제출한 적이 없어
	// RaidInventoryComponent가 활성화되지 않고, 그 결과 루팅·인벤토리·ESC 메뉴·결과 저장이 전부 죽는다.
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	// 결과 저장 ACK를 기다리는 중 접속이 끊기면 그 컨트롤러를 대기 목록에서 빼야 한다.
	// 빼지 않으면 남은 인원이 전부 ACK해도 목록이 비지 않아 레벨 전환이 영구히 막힌다.
	virtual void Logout(AController* Exiting) override;

	// 플레이어 사망 시 — 캐릭터 사망 처리에서 호출
	UFUNCTION(BlueprintCallable, Category="LS/Farming")
	void OnPlayerDied();

	// 탈출 포인트 도달 시 — ExtractionZone 액터가 호출
	UFUNCTION(BlueprintCallable, Category="LS/Farming")
	void OnExtraction();

	// UI 탈주 버튼 or 강제 종료 시 호출 — 전원 Quit으로 레이드를 끝낸다.
	UFUNCTION(BlueprintCallable, Category="LS/Farming")
	void OnQuit();

	// 개인 이탈 — 요청한 플레이어만 결과를 확정하고 자기 로비로 내보낸다. 남은 사람의 레이드는 계속된다.
	// 호스트는 혼자 빠질 수 없으므로(서버가 곧 호스트다) OnQuit()으로 전원 종료하고 다 같이 로비로 간다.
	void QuitRaidForPlayer(ALSPlayerControllerBase* QuittingPlayer);

	void NotifyRaidResultSaved(ALSPlayerControllerBase* PlayerController);

	// 다음 장착 칩이 비활성화되기까지 남은 초. 드레인 타이머가 멈춰 있으면 음수.
	// HUD 생존 링이 자체 추정 대신 실제 잔여시간을 읽도록 노출한다(서버 권한에서만 유효).
	float GetSignalGaugeDrainRemainingSeconds() const;

	// 드레인 한 주기 길이(초). 신호 링 카운트다운의 분모로 쓰인다. 배속이 걸리면 짧아진 실제 주기를 돌려준다.
	float GetSignalGaugeDrainInterval() const;

	// [디버그] 신호 게이지 드레인 배속. 1분 주기를 이 배수만큼 짧게 돌려 칩 프로토콜이 사라지는 걸 빨리 확인한다.
	// 프로토콜 디버그 콘솔에서 조정. 진행 중이면 새 배속 주기로 타이머를 다시 건다.
	void SetSignalGaugeDrainDebugSpeed(float Speed);
	float GetSignalGaugeDrainDebugSpeed() const { return SignalGaugeDrainDebugSpeed; }

private:
	void EndRaid(ELSRaidResult Result);
	void BeginRaidResultSave(ELSRaidResult Result);
	bool BuildRaidResultForPlayer(const ALSPlayerControllerBase* PlayerController, ELSRaidResult Result, TArray<FLSSessionItem>& OutInventoryItems, TArray<FLSSessionItem>& OutSafeItems, TArray<FLSSessionItem>& OutEquipmentItems, bool& bOutSaveInventory, bool& bOutSaveSafeStash, bool& bOutSaveEquipment) const;
	void HandleRaidResultSaveTimeout();
	void TravelToResultLevel();
	void ClearRaidResultSaveWait();

	// 파티를 유지한 채 레벨을 옮긴다. OpenLevel을 쓰면 안 된다 — 아래 cpp 구현의 주석 참고.
	bool ServerTravelToLevel(const TSoftObjectPtr<UWorld>& Level, const TCHAR* LevelLabel);
	// 개인 이탈자 한 명만 접속을 끊고 자기 로비로 내보낸다(호스트 세션은 그대로 유지).
	void SendPlayerToOwnLobby(ALSPlayerControllerBase* PlayerController);

	// 레이드 진입 후 1분마다 다음 장착 칩까지 빈 슬롯을 건너뛰며 신호 게이지를 감소시킨다.
	void StartSignalGaugeDrain();
	void TickSignalGaugeDrain();
	void StopSignalGaugeDrain();
	ULSSaveSubsystem* GetSaveSubsystem() const;

	FTimerHandle SignalGaugeDrainTimerHandle;

	// [디버그] 신호 게이지 드레인 배속(1 = 정상 속도). SetSignalGaugeDrainDebugSpeed에서 0.01~100으로 클램프된다.
	UPROPERTY(Transient, VisibleAnywhere, Category="LS/Farming")
	float SignalGaugeDrainDebugSpeed = 1.0f;

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

	// 개인 이탈로 결과 저장 ACK를 기다리는 컨트롤러. 전원 종료 대기(PendingRaidResultSaveControllers)와
	// 섞이면 안 된다 — 이쪽 ACK는 레벨 전환이 아니라 그 사람만 타이틀로 내보내는 신호다.
	UPROPERTY(Transient, VisibleAnywhere, Category="LS/Farming")
	TArray<TObjectPtr<ALSPlayerControllerBase>> PendingQuitControllers;

	FTimerHandle RaidResultSaveTimeoutTimerHandle;
};
