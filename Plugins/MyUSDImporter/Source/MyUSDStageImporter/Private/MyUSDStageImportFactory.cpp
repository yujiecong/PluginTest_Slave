// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyUSDStageImportFactory.h"

#include "MyUSDErrorUtils.h"
#include "MyUSDStageImporter.h"
#include "MyUSDStageImporterModule.h"
#include "MyUSDStageImportOptions.h"

#include "AssetImportTask.h"
#include "Editor.h"
#include "Modules/ModuleManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(USDStageImportFactory)

#define LOCTEXT_NAMESPACE "USDImportFactory"

UMyUsdStageImportFactory::UMyUsdStageImportFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCreateNew = false;
	bEditAfterNew = true;
	SupportedClass = UWorld::StaticClass();

	ImportPriority += 100;

	bEditorImport = true;
	bText = false;

	FModuleManager::Get().LoadModuleChecked(TEXT("UnrealUSDWrapper"));
	UnrealUSDWrapper::AddUsdImportFileFormatDescriptions(Formats);
}

UObject* UMyUsdStageImportFactory::FactoryCreateFile(
	UClass* InClass,
	UObject* InParent,
	FName InName,
	EObjectFlags Flags,
	const FString& Filename,
	const TCHAR* Parms,
	FFeedbackContext* Warn,
	bool& bOutOperationCanceled
)
{
	UObject* ImportedObject = nullptr;

	if (AssetImportTask && IsAutomatedImport())
	{
		ImportContext.ImportOptions = Cast<UMyUsdStageImportOptions>(AssetImportTask->Options);
	}

	// When importing from file we don't want to use any opened stage
	ImportContext.bReadFromStageCache = false;

#if USE_USD_SDK
	const FString InitialPackagePath = InParent ? InParent->GetName() : TEXT("/Game/");
	const bool bIsReimport = false;
	if (ImportContext.Init(InName.ToString(), Filename, InitialPackagePath, Flags, IsAutomatedImport(), bIsReimport))
	{
		GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPreImport(this, InClass, InParent, InName, Parms);

		FScopedUsdMessageLog ScopedMessageLog;

		UMyUsdStageImporter* MyUSDImporter = IMyUsdStageImporterModule::Get().GetImporter();
		MyUSDImporter->ImportFromFile(ImportContext);

		GEditor->GetEditorSubsystem<UImportSubsystem>()->BroadcastAssetPostImport(this, ImportContext.World);
		GEditor->BroadcastLevelActorListChanged();
		GEditor->RedrawLevelEditingViewports();

		ImportedObject = ImportContext.ImportedAsset ? ToRawPtr(ImportContext.ImportedAsset) : Cast<UObject>(ImportContext.SceneActor);

		ImportContext.ImportedAssets.Remove(ImportedObject);
		AdditionalImportedObjects = ImportContext.ImportedAssets;
	}
	else
	{
		bOutOperationCanceled = true;
	}

#endif	  // #if USE_USD_SDK

	return ImportedObject;
}

bool UMyUsdStageImportFactory::FactoryCanImport(const FString& Filename)
{
	const FString Extension = FPaths::GetExtension(Filename);

	for (const FString& SupportedExtension : UnrealUSDWrapper::GetAllSupportedFileFormats())
	{
		if (SupportedExtension.Equals(Extension, ESearchCase::IgnoreCase))
		{
			return true;
		}
	}

	return false;
}

void UMyUsdStageImportFactory::CleanUp()
{
	ImportContext.Reset();
	Super::CleanUp();
}

#undef LOCTEXT_NAMESPACE
