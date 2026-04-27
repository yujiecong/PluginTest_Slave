// Copyright Epic Games, Inc. All Rights Reserved.

using System;
using System.IO;
using System.Linq;

namespace UnrealBuildTool.Rules
{
	public class MyUSDStageEditor : ModuleRules
	{
		public MyUSDStageEditor(ReadOnlyTargetRules Target) : base(Target)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"DesktopPlatform",
					"DesktopWidgets",
					"EditorFramework",
					"Engine",
					"InputCore",
					"LevelEditor",
					"LevelSequence",
					"LiveLinkEditor", // For SLiveLinkSubjectRepresentationPicker
					"LiveLinkInterface", // For the ULiveLinkAnimation/TransformRole classes
					"Projects", // So that we can use the IPluginManager, required for our custom style
					"SceneOutliner",
					"Slate",
					"SlateCore",
					"UnrealEd",
					"UnrealUSDWrapper",
					"USDClasses",
					"MyUSDSchemas",
					"MyUSDStage",
					"MyUSDStageEditorViewModels",
					"MyUSDStageImporter",
					"USDUtilities",
					"WorkspaceMenuStructure",
				}
			);

			UnrealUSDWrapper.CheckAndSetupUsdSdk(Target, this);
		}
	}
}
