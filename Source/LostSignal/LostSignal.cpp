// Copyright Epic Games, Inc. All Rights Reserved.

#include "LostSignal.h"

#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "RenderCore.h"

class FLostSignalModule : public FDefaultGameModuleImpl
{
public:
	// Registers the project shader directory so custom .usf files can be compiled by UE.
	virtual void StartupModule() override
	{
		FDefaultGameModuleImpl::StartupModule();

		const FString ShaderDirectory = FPaths::Combine(FPaths::ProjectDir(), TEXT("Shaders"));
		AddShaderSourceDirectoryMapping(TEXT("/LostSignal"), ShaderDirectory);
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FLostSignalModule, LostSignal, "LostSignal");

DEFINE_LOG_CATEGORY(LogLS)
 
