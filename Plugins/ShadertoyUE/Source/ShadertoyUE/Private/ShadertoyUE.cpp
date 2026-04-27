#include "ShadertoyUE.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "SShadertoyWidget.h"

static const FName ShadertoyUETabName("ShadertoyUE");

#define LOCTEXT_NAMESPACE "FShadertoyUEModule"

void FShadertoyUEModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("ShadertoyUE"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/ProjectShaders"), PluginShaderDir);

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(ShadertoyUETabName, FOnSpawnTab::CreateRaw(this, &FShadertoyUEModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FShadertoyUETabTitle", "Shadertoy UE"))
		.SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory())
		.SetMenuType(ETabSpawnerMenuType::Enabled);
}

void FShadertoyUEModule::ShutdownModule()
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ShadertoyUETabName);
}

TSharedRef<SDockTab> FShadertoyUEModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SShadertoyWidget)
		];
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FShadertoyUEModule, ShadertoyUE)