// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class HexSystem : ModuleRules
{
	public HexSystem(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay" });
	}
}
