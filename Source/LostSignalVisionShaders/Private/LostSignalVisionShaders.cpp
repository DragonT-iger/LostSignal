#include "Modules/ModuleManager.h"

#include "Misc/Paths.h"
#include "RenderCore.h"

class FLostSignalVisionShadersModule : public IModuleInterface
{
public:
	virtual bool SupportsDynamicReloading() override
	{
		return false;
	}

	// Registers the shader virtual path before shader types are initialized.
	virtual void StartupModule() override
	{
		const FString ShaderDirectory = FPaths::Combine(FPaths::ProjectDir(), TEXT("Shaders"));
		AddShaderSourceDirectoryMapping(TEXT("/LostSignalVisionShaders"), ShaderDirectory);
	}
};

IMPLEMENT_MODULE(FLostSignalVisionShadersModule, LostSignalVisionShaders)
