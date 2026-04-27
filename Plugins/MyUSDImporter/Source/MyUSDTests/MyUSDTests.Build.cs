// Copyright Epic Games, Inc. All Rights Reserved.

using System;
using System.IO;
using System.Linq;

namespace UnrealBuildTool.Rules
{
	public class MyUSDTests : ModuleRules
	{
		public MyUSDTests(ReadOnlyTargetRules Target) : base(Target)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"Engine",
					"UnrealUSDWrapper",
					"USDClasses",
					"MyUSDSchemas",
					"MyUSDStage",
					"USDUtilities",
				}
			);

			if (Target.bBuildEditor)
			{
				PrivateDependencyModuleNames.AddRange(
					new string[]
					{
						"BlueprintGraph",
						"UnrealEd",
					}
				);
			}

			UnrealUSDWrapper.CheckAndSetupUsdSdk(Target, this);
		}
	}
}
