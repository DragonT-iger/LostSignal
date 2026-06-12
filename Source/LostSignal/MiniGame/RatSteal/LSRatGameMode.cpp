#include "MiniGame/RatSteal/LSRatGameMode.h"

#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "LostSignal.h"
#include "MiniGame/RatSteal/LSRatPlayer.h"
#include "MiniGame/RatSteal/LSRatPlayerController.h"
#include "MiniGame/RatSteal/LSRatStealSubsystem.h"
#include "MiniGame/RatSteal/UI/LSRatHUDWidget.h"
#include "MiniGame/RatSteal/UI/LSRatPauseWidget.h"
#include "MiniGame/RatSteal/UI/LSRatResultWidget.h"

ALSRatGameMode::ALSRatGameMode()
{
	PrimaryActorTick.bCanEverTick = true;

	DefaultPawnClass = ALSRatPlayer::StaticClass();
	PlayerControllerClass = ALSRatPlayerController::StaticClass();
}

void ALSRatGameMode::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogLS, Log, TEXT("[RatSteal] GameMode BeginPlay: DefaultPawn=%s PlayerController=%s"),
		*GetNameSafe(DefaultPawnClass),
		*GetNameSafe(PlayerControllerClass));

	RemainingTime = RoundDuration;
	ReadyTimer = ReadyDuration;

	// WBP 미할당이면 C++ 위젯 클래스 그대로 사용 (폴백 레이아웃 내장)
	if (!HUDWidgetClass)
	{
		HUDWidgetClass = ULSRatHUDWidget::StaticClass();
	}

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		HUDWidget = CreateWidget<ULSRatHUDWidget>(PC, HUDWidgetClass);
		if (HUDWidget)
		{
			HUDWidget->AddToViewport();
		}
	}

	SetPhase(ELSRatPhase::Ready);
}

void ALSRatGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	switch (Phase)
	{
	case ELSRatPhase::Ready:
		ReadyTimer -= DeltaSeconds;
		if (ReadyTimer <= 0.f)
		{
			SetPhase(ELSRatPhase::Playing);
		}
		break;

	case ELSRatPhase::Playing:
		if (bTimerEnabled)
		{
			RemainingTime -= DeltaSeconds;
			if (RemainingTime <= 0.f)
			{
				RemainingTime = 0.f;
				EndGame(ELSRatEndReason::TimeUp);
			}
		}
		break;

	default:
		break;
	}
}

int32 ALSRatGameMode::SubmitInventory(const TArray<FLSRatSlotData>& Datas)
{
	// 원작 GameManager::ReceiveScore — 카운트에 크기가 이미 반영돼 있다 (S+1/M+6/L+9)
	int32 CurEggplant = 0;
	int32 CurPotato = 0;
	int32 CurPumpkin = 0;

	for (const FLSRatSlotData& Data : Datas)
	{
		switch (Data.Type)
		{
		case ELSRatCropType::Eggplant: CurEggplant += Data.Count; break;
		case ELSRatCropType::Potato:   CurPotato += Data.Count;   break;
		case ELSRatCropType::Pumpkin:  CurPumpkin += Data.Count;  break;
		default: break;
		}
	}

	EggplantCount += CurEggplant;
	PotatoCount += CurPotato;
	PumpkinCount += CurPumpkin;

	const int32 CurScore =
		CurEggplant * LSRat::GetScorePerCount(ELSRatCropType::Eggplant) +
		CurPotato * LSRat::GetScorePerCount(ELSRatCropType::Potato) +
		CurPumpkin * LSRat::GetScorePerCount(ELSRatCropType::Pumpkin);

	if (CurScore > 0)
	{
		TotalScore += CurScore;
		OnScoreChanged.Broadcast(TotalScore, CurScore);
		UE_LOG(LogLS, Log, TEXT("[RatSteal] 제출 +%d점 (총 %d점)"), CurScore, TotalScore);
	}

	return CurScore;
}

void ALSRatGameMode::EndGame(ELSRatEndReason Reason)
{
	if (Phase == ELSRatPhase::End)
	{
		return;
	}

	SetPhase(ELSRatPhase::End);

	FLSRatResult Result;
	Result.EndReason = Reason;
	Result.TotalScore = TotalScore;
	Result.EggplantCount = EggplantCount;
	Result.PotatoCount = PotatoCount;
	Result.PumpkinCount = PumpkinCount;
	// 등급은 생존 종료에만 부여. 패배 시 정책 미정(02_Progression) — 현재는 0
	Result.Stars = (Reason == ELSRatEndReason::TimeUp) ? ComputeStars(TotalScore) : 0;

	if (UGameInstance* GI = GetGameInstance())
	{
		if (ULSRatStealSubsystem* Subsystem = GI->GetSubsystem<ULSRatStealSubsystem>())
		{
			Subsystem->StoreResult(Result);
		}
	}

	OnGameEnded.Broadcast(Result);
	UE_LOG(LogLS, Log, TEXT("[RatSteal] 종료: %s, 점수 %d, ★%d"),
		*UEnum::GetValueAsString(Reason), Result.TotalScore, Result.Stars);

	if (!ResultWidgetClass)
	{
		ResultWidgetClass = ULSRatResultWidget::StaticClass();
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (PC)
	{
		ResultWidget = CreateWidget<ULSRatResultWidget>(PC, ResultWidgetClass);
		if (ResultWidget)
		{
			ResultWidget->SetResult(Result);
			ResultWidget->AddToViewport(10);
		}
	}

	if (PC)
	{
		PC->SetShowMouseCursor(true);
		// Enter/Space 복귀 입력이 컨트롤러에 닿아야 하므로 GameAndUI
		PC->SetInputMode(FInputModeGameAndUI());
	}
	UGameplayStatics::SetGamePaused(this, true);
}

void ALSRatGameMode::TogglePause()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);

	if (Phase == ELSRatPhase::Playing)
	{
		SetPhase(ELSRatPhase::Paused);
		UGameplayStatics::SetGamePaused(this, true);

		if (!PauseWidgetClass)
		{
			PauseWidgetClass = ULSRatPauseWidget::StaticClass();
		}
		if (PC)
		{
			PauseWidget = CreateWidget<ULSRatPauseWidget>(PC, PauseWidgetClass);
			if (PauseWidget)
			{
				PauseWidget->AddToViewport(20);
			}
		}
		if (PC)
		{
			PC->SetShowMouseCursor(true);
		}
	}
	else if (Phase == ELSRatPhase::Paused)
	{
		SetPhase(ELSRatPhase::Playing);
		UGameplayStatics::SetGamePaused(this, false);

		if (PauseWidget)
		{
			PauseWidget->RemoveFromParent();
			PauseWidget = nullptr;
		}
		if (PC)
		{
			PC->SetShowMouseCursor(false);
		}
	}
}

void ALSRatGameMode::SetPhase(ELSRatPhase NewPhase)
{
	if (Phase == NewPhase && NewPhase != ELSRatPhase::Ready)
	{
		return;
	}

	Phase = NewPhase;
	OnPhaseChanged.Broadcast(Phase);
}

int32 ALSRatGameMode::ComputeStars(int32 Score) const
{
	int32 Stars = 0;
	for (const int32 Cut : StarScoreCuts)
	{
		if (Score >= Cut)
		{
			Stars++;
		}
	}
	return Stars;
}
