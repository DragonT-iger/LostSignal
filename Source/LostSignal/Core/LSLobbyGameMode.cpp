#include "Core/LSLobbyGameMode.h"
#include "Session/LSSaveSubsystem.h"
#include "Session/LSSessionSubsystem.h"
#include "Session/LSSessionSettings.h"
#include "LostSignal.h"
#include "Kismet/GameplayStatics.h"

void ALSLobbyGameMode::StartRaid()
{
	ULSSessionSubsystem* SessionSub = GetGameInstance()->GetSubsystem<ULSSessionSubsystem>();
	if (!SessionSub) return;

	// 로드아웃은 인벤토리 시스템 구현 후 연결 — 지금은 빈 상태로 시작
	const ULSSaveSubsystem* SaveSub = GetGameInstance()->GetSubsystem<ULSSaveSubsystem>();
	if (SaveSub)
	{
		SessionSub->StartRaid(SaveSub->GetStash());
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] SaveSubsystem is missing. Starting raid with an empty inventory."));
		SessionSub->StartRaid({});
	}

	const ULSSessionSettings* Settings = GetDefault<ULSSessionSettings>();
	if (!Settings->FarmingLevel.IsNull())
	{
		UGameplayStatics::OpenLevelBySoftObjectPtr(this, Settings->FarmingLevel);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("[Lobby] FarmingLevel 미설정 - 프로젝트 설정 > LS Session Settings 확인"));
	}
}
