// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LSGameModeBase.generated.h"

/**
 * Base GameMode for LostSignal.
 * Default class wiring should be set in the derived Blueprint.
 */
UCLASS(Abstract)
class ALSGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:

	ALSGameModeBase();
};
