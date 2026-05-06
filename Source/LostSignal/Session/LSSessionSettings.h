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
	TSoftObjectPtr<UWorld> LobbyLevel;

	UPROPERTY(config, EditAnywhere, Category="Levels")
	TSoftObjectPtr<UWorld> ResultLevel;
};
