#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FPluginSimpleModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);
	void AddMenuBarExtension(class FMenuBarBuilder& Builder);
	void FillPulldownMenu(class FMenuBuilder& MenuBuilder);
	void OpenDemoTab();
	void OpenDemoWindow();

	TSharedPtr<class FExtender> MenuExtender;
};
