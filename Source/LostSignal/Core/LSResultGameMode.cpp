#include "Core/LSResultGameMode.h"
#include "Session/LSSessionSettings.h"
#include "LostSignal.h"
#include "Kismet/GameplayStatics.h"

void ALSResultGameMode::ReturnToLobby()
{
	const ULSSessionSettings* Settings = GetDefault<ULSSessionSettings>();
	if (!Settings->LobbyLevel.IsNull())
	{
		UGameplayStatics::OpenLevelBySoftObjectPtr(this, Settings->LobbyLevel);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("[Result] LobbyLevel 미설정 - 프로젝트 설정 > LS Session Settings 확인"));
	}
}
