// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LSGameMode.generated.h"

/**
 * Base GameMode for LostSignal.
 * Default class wiring should be set in the derived Blueprint.
 */
UCLASS(Abstract)
class ALSGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	ALSGameMode();
};
