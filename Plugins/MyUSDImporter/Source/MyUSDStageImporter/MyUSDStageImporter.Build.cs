// Copyright Epic Games, Inc. All Rights Reserved.

using System;
using System.IO;
using System.Linq;

namespace UnrealBuildTool.Rules
{
	public class MyUSDStageImporter : ModuleRules
	{
		public MyUSDStageImporter(ReadOnlyTargetRules Target) : base(Target)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"ApplicationCore",
					"Core",
					"CoreUObject",
					"EditorFramework",
					"EditorStyle", // For the font style on the stage actor customization
					"Engine",
					"GeometryCache",
					"InputCore",
					"JsonUtilities",
					"LevelSequence",
					"MainFrame",
					"MessageLog",
					"MovieScene",
					"PropertyEditor", // For the import options's details customization
					"RenderCore", // So that we can release resources of reimported meshes
					"Slate",
					"SlateCore",
					"UnrealEd",
					"UnrealUSDWrapper",
					"USDClasses",
					"MyUSDSchemas",
					"MyUSDStage",
					"USDUtilities",
				}
			);

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"HairStrandsCore",
				}
			);

			UnrealUSDWrapper.CheckAndSetupUsdSdk(Target, this);
		}
	}
}
