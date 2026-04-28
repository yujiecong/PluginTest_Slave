#include "PluginSimple.h"

#include "Framework/Docking/TabManager.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/Application/SlateApplication.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"

static const FName PluginTabName("PluginSimple");

void FPluginSimpleModule::StartupModule()
{
	FGlobalTabmanager::Get()
		->RegisterNomadTabSpawner(PluginTabName, FOnSpawnTab::CreateRaw(this, &FPluginSimpleModule::OnSpawnPluginTab))
		.SetDisplayName(FText::FromString(TEXT("Plugin Simple")))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	MenuExtender = MakeShared<FExtender>();
	MenuExtender->AddMenuBarExtension(
		"Help",
		EExtensionHook::After,
		nullptr,
		FMenuBarExtensionDelegate::CreateRaw(this, &FPluginSimpleModule::AddMenuBarExtension)
	);

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
}

void FPluginSimpleModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("LevelEditor"))
	{
		FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
		if (MenuExtender.IsValid())
		{
			LevelEditorModule.GetMenuExtensibilityManager()->RemoveExtender(MenuExtender);
			MenuExtender.Reset();
		}
	}

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(PluginTabName);
}

TSharedRef<SDockTab> FPluginSimpleModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("Hello from Plugin Simple (Dockable Tab)!")))
			]
		];
}

void FPluginSimpleModule::AddMenuBarExtension(FMenuBarBuilder& Builder)
{
	Builder.AddPullDownMenu(
		FText::FromString(TEXT("PluginSimple")),
		FText::FromString(TEXT("Open PluginSimple menu")),
		FNewMenuDelegate::CreateRaw(this, &FPluginSimpleModule::FillPulldownMenu),
		"PluginSimple"
	);
}

void FPluginSimpleModule::FillPulldownMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Open Demo Tab")),
		FText::FromString(TEXT("Open the dockable demo tab")),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FPluginSimpleModule::OpenDemoTab))
	);

	MenuBuilder.AddMenuEntry(
		FText::FromString(TEXT("Open Demo Window")),
		FText::FromString(TEXT("Open the separate demo window")),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FPluginSimpleModule::OpenDemoWindow))
	);
}

void FPluginSimpleModule::OpenDemoTab()
{
	FGlobalTabmanager::Get()->TryInvokeTab(PluginTabName);
}

void FPluginSimpleModule::OpenDemoWindow()
{
	TSharedRef<SWindow> NewWindow = SNew(SWindow)
		.Title(FText::FromString(TEXT("Plugin Simple Window")))
		.ClientSize(FVector2D(480, 200))
		.SupportsMinimize(false)
		.SupportsMaximize(false)
		[
			SNew(SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(FText::FromString(TEXT("Hello from Plugin Simple (Window)!")))
			]
		];

	FSlateApplication::Get().AddWindow(NewWindow);
}

IMPLEMENT_MODULE(FPluginSimpleModule, PluginSimple)
