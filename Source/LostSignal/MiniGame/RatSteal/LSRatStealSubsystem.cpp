#include "MiniGame/RatSteal/LSRatStealSubsystem.h"

#include "Kismet/GameplayStatics.h"
#include "LostSignal.h"

void ULSRatStealSubsystem::EnterMiniGame(const TSoftObjectPtr<UWorld>& MiniGameLevel, APawn* Interactor)
{
	if (MiniGameLevel.IsNull())
	{
		UE_LOG(LogLS, Warning, TEXT("[RatSteal] EnterMiniGame: MiniGameLevel이 비어 있음"));
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ReturnMapName = FName(*UGameplayStatics::GetCurrentLevelName(World, true));
	ReturnTransform = Interactor ? Interactor->GetActorTransform() : FTransform::Identity;
	bHasPendingReturn = true;
	LastResult = FLSRatResult();

	UE_LOG(LogLS, Log, TEXT("[RatSteal] 미니게임 진입. 복귀 맵: %s"), *ReturnMapName.ToString());
	UGameplayStatics::OpenLevelBySoftObjectPtr(World, MiniGameLevel);
}

void ULSRatStealSubsystem::ReturnToMainWorld()
{
	if (ReturnMapName.IsNone())
	{
		UE_LOG(LogLS, Warning, TEXT("[RatSteal] ReturnToMainWorld: 복귀 맵이 저장돼 있지 않음"));
		return;
	}

	UE_LOG(LogLS, Log, TEXT("[RatSteal] 본편 복귀: %s (점수 %d)"), *ReturnMapName.ToString(), LastResult.TotalScore);
	UGameplayStatics::OpenLevel(GetWorld(), ReturnMapName);
}

void ULSRatStealSubsystem::StoreResult(const FLSRatResult& Result)
{
	LastResult = Result;
}

bool ULSRatStealSubsystem::ConsumeReturnTransform(FTransform& OutTransform)
{
	if (!bHasPendingReturn)
	{
		return false;
	}

	OutTransform = ReturnTransform;
	bHasPendingReturn = false;
	return true;
}
