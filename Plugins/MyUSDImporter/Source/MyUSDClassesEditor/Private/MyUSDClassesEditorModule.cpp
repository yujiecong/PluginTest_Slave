// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyUSDClassesEditorModule.h"

#include "MyUSDAssetCache2.h"

#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "MyUSDClassesEditorModule"

UMyUsdAssetCache2* IMyUsdClassesEditorModule::ShowMissingDefaultAssetCacheDialog()
{
	return nullptr;
}

void IMyUsdClassesEditorModule::ShowMissingDefaultAssetCacheDialog(UMyUsdAssetCache2*& OutCreatedCache, bool& bOutUserAccepted)
{
}

EDefaultAssetCacheDialogOption IMyUsdClassesEditorModule::ShowMissingDefaultAssetCacheDialog(UMyUsdAssetCache2*& OutCreatedCache)
{
	return EDefaultAssetCacheDialogOption::Cancel;
}

class FMyUsdClassesEditorModule : public IMyUsdClassesEditorModule
{
public:
	virtual void StartupModule() override
	{
	}

	virtual void ShutdownModule() override
	{
	}
};

IMPLEMENT_MODULE(FMyUsdClassesEditorModule, MyUSDClassesEditor);

#undef LOCTEXT_NAMESPACE
