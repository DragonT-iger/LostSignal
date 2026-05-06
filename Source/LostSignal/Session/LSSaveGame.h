#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Session/LSSessionSubsystem.h"
#include "LSSaveGame.generated.h"

UCLASS()
class LOSTSIGNAL_API ULSSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY() TArray<FLSSessionItem> Stash;
};
