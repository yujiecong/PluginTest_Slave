// Copyright Epic Games, Inc. All Rights Reserved.

using System;
using System.IO;

namespace UnrealBuildTool.Rules
{
	public class MyUSDClassesEditor : ModuleRules
	{
		public MyUSDClassesEditor(ReadOnlyTargetRules Target) : base(Target)
		{
			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"AssetDefinition",
					"Core",
					"CoreUObject",
					"Engine",
					"Slate",
					"SlateCore"
				}
			);

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"ContentBrowser",
					"InputCore",
					"UnrealEd",
					"USDClasses",
				}
			);
		}
	}
}
