// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/LSGameModeBase.h"
#include "Core/LSPlayerControllerBase.h"
#include "Characters/LSCharacterBase.h"

ALSGameModeBase::ALSGameModeBase()
{
	bUseSeamlessTravel = true;

	//PlayerControllerClass = ALSPlayerControllerBase::StaticClass();
	//DefaultPawnClass = ALSCharacterBase::StaticClass();
}
