#include "Core/LSResultGameMode.h"
#include "Session/LSSessionSettings.h"
#include "LostSignal.h"
#include "Engine/World.h"

void ALSResultGameMode::ReturnToLobby()
{
	// OpenLevel이 아니라 ServerTravel이어야 파티가 유지된 채 로비로 돌아온다.
	// (OpenLevel은 넷드라이버를 파괴하고 LastURL의 Listen 옵션까지 떼어낸다)
	const ULSSessionSettings* Settings = GetDefault<ULSSessionSettings>();
	UWorld* World = GetWorld();
	const FString LobbyPath = (Settings && !Settings->LobbyLevel.IsNull())
		? Settings->LobbyLevel.ToSoftObjectPath().GetLongPackageName()
		: FString();
	if (!World || LobbyPath.IsEmpty())
	{
		UE_LOG(LogLS, Warning, TEXT("[Result] LobbyLevel 미설정 - 프로젝트 설정 > LS Session Settings 확인"));
		return;
	}

	if (!World->ServerTravel(LobbyPath))
	{
		UE_LOG(LogLS, Warning, TEXT("[Result] LobbyLevel ServerTravel 실패: %s"), *LobbyPath);
	}
}
