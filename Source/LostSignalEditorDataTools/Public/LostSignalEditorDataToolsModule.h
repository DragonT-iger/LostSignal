#pragma once

#include "Modules/ModuleManager.h"

class SDockTab;
class FSpawnTabArgs;

class FLostSignalEditorDataToolsModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RegisterMenus();
	void ReimportDataTables();
	void OpenMapTileEditor();
	TSharedRef<SDockTab> SpawnMapTileEditorTab(const FSpawnTabArgs& SpawnTabArgs);
};
