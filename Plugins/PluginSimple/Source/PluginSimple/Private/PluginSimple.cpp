#include "PluginSimple.h"
#include "LevelEditor.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SVerticalBox.h"
#include "Widgets/SWindow.h"
#include "Framework/Application/SlateApplication.h"
#include "Modules/ModuleManager.h"

static const FName PluginTabName("PluginSimpleTab");

void FPluginSimpleModule::StartupModule()
{
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(PluginTabName, FOnSpawnTab::CreateRaw(this, &FPluginSimpleModule::OnSpawnPluginTab))
        .SetDisplayName(FText::FromString("Plugin Simple"))
        .SetMenuType(ETabSpawnerMenuType::Hidden);

    MenuExtender = MakeShareable(new FExtender());
    MenuExtender->AddMenuBarExtension("Help", EExtensionHook::After, nullptr, FMenuBarExtensionDelegate::CreateRaw(this, &FPluginSimpleModule::AddMenuBarExtension));

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
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().Padding(4)
                [
                    SNew(STextBlock).Text(FText::FromString("Hello from Plugin Simple (Dockable Tab)!"))
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(4)
                [
                    SNew(SButton)
                    .Text(FText::FromString("Click Me"))
                    .OnClicked_Lambda([]() { return FReply::Handled(); })
                ]
            ]
        ];
}

void FPluginSimpleModule::AddMenuBarExtension(FMenuBarBuilder& Builder)
{
    Builder.AddPullDownMenu(
        FText::FromString("PluginSimple"),
        FText::FromString("Open PluginSimple menu"),
        FNewMenuDelegate::CreateRaw(this, &FPluginSimpleModule::FillPulldownMenu),
        "PluginSimple"
    );
}

void FPluginSimpleModule::FillPulldownMenu(FMenuBuilder& MenuBuilder)
{
    MenuBuilder.AddMenuEntry(
        FText::FromString("Open Demo Tab"),
        FText::FromString("Open the dockable demo tab"),
        FSlateIcon(),
        FUIAction(FExecuteAction::CreateRaw(this, &FPluginSimpleModule::OpenDemoTab))
    );

    MenuBuilder.AddMenuEntry(
        FText::FromString("Open Demo Window"),
        FText::FromString("Open the separate demo window"),
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
        .Title(FText::FromString("Plugin Simple Window"))
        .ClientSize(FVector2D(480, 200))
        .SupportsMinimize(false)
        .SupportsMaximize(false)
        [
            SNew(SBox)
            .HAlign(HAlign_Center)
            .VAlign(VAlign_Center)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().Padding(4)
                [
                    SNew(STextBlock).Text(FText::FromString("Hello from Plugin Simple (Window)!"))
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(4)
                [
                    SNew(SButton)
                    .Text(FText::FromString("Close"))
                    .OnClicked_Lambda([NewWindow]() -> FReply { FSlateApplication::Get().RequestDestroyWindow(NewWindow); return FReply::Handled(); })
                ]
            ]
        ];

    FSlateApplication::Get().AddWindow(NewWindow);
}

IMPLEMENT_MODULE(FPluginSimpleModule, PluginSimple)
