#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MiniGame/RatSteal/LSRatTypes.h"
#include "LSRatGameMode.generated.h"

class ULSRatHUDWidget;
class ULSRatPauseWidget;
class ULSRatResultWidget;
class UAudioComponent;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLSRatOnScoreChanged, int32, TotalScore, int32, DeltaScore);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLSRatOnPhaseChanged, ELSRatPhase, NewPhase);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLSRatOnGameEnded, const FLSRatResult&, Result);

/**
 * RatSteal 규칙/점수/타이머/종료 (구 GameManager, 22_System_Timer / 21_System_Score).
 * 제한 시간 3분 스코어 어택. 승리 조건 없음, 패배는 BabyStarved / PlayerDead.
 */
UCLASS()
class LOSTSIGNAL_API ALSRatGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ALSRatGameMode();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** 제출 정산 (구 ReceiveScore). 반환값 = 이번 제출 점수 = 베이비 회복량 */
	UFUNCTION(BlueprintCallable, Category = "LS/RatSteal")
	int32 SubmitInventory(const TArray<FLSRatSlotData>& Datas);

	UFUNCTION(BlueprintCallable, Category = "LS/RatSteal")
	void EndGame(ELSRatEndReason Reason);

	UFUNCTION(BlueprintCallable, Category = "LS/RatSteal")
	void TogglePause();

	UFUNCTION(BlueprintPure, Category = "LS/RatSteal")
	ELSRatPhase GetPhase() const { return Phase; }

	UFUNCTION(BlueprintPure, Category = "LS/RatSteal")
	bool IsPlaying() const { return Phase == ELSRatPhase::Playing; }

	UFUNCTION(BlueprintPure, Category = "LS/RatSteal")
	float GetRemainingTime() const { return RemainingTime; }

	UFUNCTION(BlueprintPure, Category = "LS/RatSteal")
	int32 GetTotalScore() const { return TotalScore; }

	/** 튜토리얼 등에서 포만 감소를 끌 때 사용 (32_Tutorial) */
	UFUNCTION(BlueprintPure, Category = "LS/RatSteal")
	bool IsFullnessDecayEnabled() const { return bFullnessDecayEnabled; }

	UPROPERTY(BlueprintAssignable, Category = "LS/RatSteal")
	FLSRatOnScoreChanged OnScoreChanged;

	UPROPERTY(BlueprintAssignable, Category = "LS/RatSteal")
	FLSRatOnPhaseChanged OnPhaseChanged;

	UPROPERTY(BlueprintAssignable, Category = "LS/RatSteal")
	FLSRatOnGameEnded OnGameEnded;

protected:
	void SetPhase(ELSRatPhase NewPhase);
	int32 ComputeStars(int32 Score) const;
	void StartBgm();
	void PlayBgm(USoundBase* Sound);
	void StopBgm();
	void UpdateBgmByFarmerState();
	bool ShouldUseFarmerNearBgm() const;

	UFUNCTION()
	void HandleBgmFinished();

	/** 제한 시간 3분 (50_Content_Balance 확정) */
	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Balance", meta = (ClampMin = 1))
	float RoundDuration = 180.f;

	/** Ready 카운트다운 길이 (원작에 없음 — 길이 미정, 플레이로 조정) */
	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Balance", meta = (ClampMin = 0))
	float ReadyDuration = 3.f;

	/** 생존 종료 시 ★ 등급 컷 (신규 — 컷 수치 미정, 밸런스 후 확정. 02_Progression) */
	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Balance")
	TArray<int32> StarScoreCuts = { 500, 1500, 3000 };

	/** 3분 타이머 사용 여부 (튜토리얼은 끔) */
	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Balance")
	bool bTimerEnabled = true;

	/** 포만 감소 사용 여부 (튜토리얼은 끔) */
	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Balance")
	bool bFullnessDecayEnabled = true;

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|UI")
	TSubclassOf<ULSRatHUDWidget> HUDWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|UI")
	TSubclassOf<ULSRatResultWidget> ResultWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|UI")
	TSubclassOf<ULSRatPauseWidget> PauseWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Audio")
	TObjectPtr<USoundBase> BgmSound;

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Audio")
	TObjectPtr<USoundBase> FarmerNearBgmSound;

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Audio", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float BgmVolume = 0.45f;

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Audio", meta = (ClampMin = 0.0))
	float FarmerNearBgmDistance = 650.f;

	UPROPERTY()
	TObjectPtr<ULSRatHUDWidget> HUDWidget;

	UPROPERTY()
	TObjectPtr<ULSRatResultWidget> ResultWidget;

	UPROPERTY()
	TObjectPtr<ULSRatPauseWidget> PauseWidget;

	UPROPERTY()
	TObjectPtr<UAudioComponent> BgmAudioComponent;

	UPROPERTY()
	TObjectPtr<USoundBase> CurrentBgmSound;

private:
	ELSRatPhase Phase = ELSRatPhase::Ready;
	float RemainingTime = 0.f;
	float ReadyTimer = 0.f;

	// 누적 집계 (구 ep_count/pt_count/pk_count/totalscore)
	int32 TotalScore = 0;
	int32 EggplantCount = 0;
	int32 PotatoCount = 0;
	int32 PumpkinCount = 0;
};
