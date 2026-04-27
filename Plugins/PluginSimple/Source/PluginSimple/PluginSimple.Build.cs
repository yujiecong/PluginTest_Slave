using UnrealBuildTool;
using System.Collections.Generic;

public class PluginSimple : ModuleRules
{
    public PluginSimple(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "EditorStyle"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "LevelEditor",
                "UnrealEd"
            }
        );
    }
}
