// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MyGeometryCacheUSD : ModuleRules
{
	public MyGeometryCacheUSD(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"GeometryCache",
				"GeometryCacheStreamer",
				"RenderCore",
				"RHI",
				"UnrealUSDWrapper",
				"USDUtilities"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[] {
				"USDClasses",
            }
        );

		PrivateIncludePathModuleNames.Add("DerivedDataCache");
	}
}
