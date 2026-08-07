// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class LostSignal : ModuleRules
{
	public LostSignal(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"AnimGraphRuntime",
			"NavigationSystem",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"Niagara",
			"UMG",
			"Slate",
			"SlateCore",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"RenderCore",
			"RHI",
			"Renderer",
			"Projects",
			"DeveloperSettings",
			"Json",
			"JsonUtilities",
			"Paper2D",
			"LostSignalVisionShaders"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"LostSignalVisionShaders",
			// 초대 코드에 넣을 호스트 주소·리슨 포트를 읽는다 (LSInviteCode).
			"Sockets"
		});

		PublicIncludePaths.AddRange(new string[] {
			"LostSignal"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
