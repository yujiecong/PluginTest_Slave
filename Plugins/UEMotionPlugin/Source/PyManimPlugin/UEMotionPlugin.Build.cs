using UnrealBuildTool;

public class UEMotionPlugin : ModuleRules
{
	public UEMotionPlugin(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"Slate",
			"SlateCore",
			"UnrealEd",
			"PythonScriptPlugin",
			"ProceduralMeshComponent",
			"ImageWriteQueue",
			"MovieRenderPipelineCore",
			"MovieRenderPipelineRenderPasses",
			"MovieRenderPipelineEditor",
			"CinematicCamera",
			"LevelSequence",
			"MovieScene",
			"MovieSceneTracks",
			"TimeManagement"
		});

		PrivateIncludePaths.AddRange(new string[] { "UEMotionPlugin/Private" });
	}
}
