#pragma once

#include "CoreMinimal.h"
#include "Core/LSGameModeBase.h"
#include "Session/LSSessionSubsystem.h"
#include "LSFarmingGameMode.generated.h"

UCLASS()
class LOSTSIGNAL_API ALSFarmingGameMode : public ALSGameModeBase
{
	GENERATED_BODY()

public:
	// 플레이어 사망 시 — 캐릭터 사망 처리에서 호출
	UFUNCTION(BlueprintCallable, Category="LS/Farming")
	void OnPlayerDied();

	// 탈출 포인트 도달 시 — ExtractionZone 액터가 호출
	UFUNCTION(BlueprintCallable, Category="LS/Farming")
	void OnExtraction();

	// UI 탈주 버튼 or 강제 종료 시 호출
	UFUNCTION(BlueprintCallable, Category="LS/Farming")
	void OnQuit();

private:
	void EndRaid(ELSRaidResult Result);

	bool bRaidEnded = false;
};
