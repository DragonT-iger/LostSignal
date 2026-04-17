using UnrealBuildTool;

public class LostSignalEditorAddComponentTool : ModuleRules
{
    public LostSignalEditorAddComponentTool(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
           "CoreUObject",
            "Engine",
            "Slate",
            "SlateCore",
            "UnrealEd",
            "LevelEditor",
            "ToolMenus",
            "LostSignal"
        });
    }
}
