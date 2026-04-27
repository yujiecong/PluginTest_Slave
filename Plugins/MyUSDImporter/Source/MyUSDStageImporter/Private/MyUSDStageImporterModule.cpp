// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyUSDStageImporterModule.h"

#include "UnrealUSDWrapper.h"
#include "USDMemory.h"
#include "MyUSDStageImporter.h"
#include "MyUSDStageImportOptionsCustomization.h"

#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Templates/UniquePtr.h"

#define LOCTEXT_NAMESPACE "UsdStageImporterModule"

class FMyUSDStageImporterModule : public IMyUsdStageImporterModule
{
public:
	virtual void StartupModule() override
	{
#if USE_USD_SDK
		LLM_SCOPE_BYTAG(Usd);

		FModuleManager::Get().LoadModuleChecked<IUnrealUSDWrapperModule>(TEXT("UnrealUSDWrapper"));

		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
		PropertyModule.RegisterCustomClassLayout(
			TEXT("UsdStageImportOptions"),
			FOnGetDetailCustomizationInstance::CreateStatic(&FMyUsdStageImportOptionsCustomization::MakeInstance)
		);

		MyUSDStageImporter = MakeUnique<UMyUsdStageImporter>();
#endif	  // #if USE_USD_SDK
	}

	virtual void ShutdownModule() override
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
		PropertyModule.UnregisterCustomClassLayout(TEXT("UsdStageImportOptions"));

		MyUSDStageImporter.Reset();
	}

	class UMyUsdStageImporter* GetImporter() override
	{
		return MyUSDStageImporter.Get();
	}

private:
	TUniquePtr<UMyUsdStageImporter> MyUSDStageImporter;
};

IMPLEMENT_MODULE_USD(FMyUSDStageImporterModule, MyUSDStageImporter);

#undef LOCTEXT_NAMESPACE
