#include "Core/LSFarmingGameMode.h"
#include "Session/LSSessionSubsystem.h"
#include "LostSignal.h"

void ALSFarmingGameMode::OnPlayerDied()
{
	EndRaid(ELSRaidResult::Dead);
}

void ALSFarmingGameMode::OnExtraction()
{
	EndRaid(ELSRaidResult::Extracted);
}

void ALSFarmingGameMode::OnQuit()
{
	EndRaid(ELSRaidResult::Quit);
}

void ALSFarmingGameMode::EndRaid(ELSRaidResult Result)
{
	if (bRaidEnded) return;
	bRaidEnded = true;

	ULSSessionSubsystem* SessionSub = GetGameInstance()->GetSubsystem<ULSSessionSubsystem>();
	if (!SessionSub) return;

	SessionSub->EndRaid(Result);
}
