#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "LSSessionSettings.generated.h"

UCLASS(config=Game, defaultconfig, meta=(DisplayName="LS Session Settings"))
class LOSTSIGNAL_API ULSSessionSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(config, EditAnywhere, Category="Levels")
	TSoftObjectPtr<UWorld> TitleLevel;

	UPROPERTY(config, EditAnywhere, Category="Levels")
	TSoftObjectPtr<UWorld> LobbyLevel;

	UPROPERTY(config, EditAnywhere, Category="Levels")
	TSoftObjectPtr<UWorld> FarmingLevel;

	UPROPERTY(config, EditAnywhere, Category="Levels")
	TSoftObjectPtr<UWorld> ResultLevel;

	// 디버그용 레이드 테스트 레벨. 프로토콜 디버그 패널의 "테스트 맵 가기" 버튼이 정식 레이드 진입
	// 경로를 그대로 타되 목적지만 이 레벨로 바꿔 진입한다. World Settings 의 GameMode 는
	// FarmingLevel 과 동일하게 BP_LSFarmingGameMode 로 지정해야 드레인 등 레이드 환경이 동일하다.
	UPROPERTY(config, EditAnywhere, Category="Levels")
	TSoftObjectPtr<UWorld> TestRaidLevel;
};
