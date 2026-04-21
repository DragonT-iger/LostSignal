#pragma once

#include "Modules/ModuleManager.h"

class FLostSignalEditorAddComponentToolModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RegisterMenus();
	void AddVisionSetupToSelectedActors();
	void AddRoofSetupToSelectedActors();
};
