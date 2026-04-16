using UnrealBuildTool;

public class LostSignalVisionShaders : ModuleRules
{
	public LostSignalVisionShaders(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"RenderCore",
			"RHI",
			"Renderer",
			"Projects"
		});
	}
}
