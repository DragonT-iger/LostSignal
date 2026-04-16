// Copyright Epic Games, Inc. All Rights Reserved.

#include "LostSignal.h"

#include "Modules/ModuleManager.h"

class FLostSignalModule : public FDefaultGameModuleImpl
{
};

IMPLEMENT_PRIMARY_GAME_MODULE(FLostSignalModule, LostSignal, "LostSignal");

DEFINE_LOG_CATEGORY(LogLS)
 
