// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyUSDStageEditorBlueprintLibrary.h"

#include "MyUSDStageActor.h"
#include "MyUSDStageEditorModule.h"
#include "MyUsdWrappers/SdfLayer.h"
#include "UsdWrappers/MyUsdPrim.h"

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MyUSDStageEditorBlueprintLibrary)

bool UMyUsdStageEditorBlueprintLibrary::OpenStageEditor()
{
	IMyUsdStageEditorModule& StageEditorModule = FModuleManager::GetModuleChecked<IMyUsdStageEditorModule>("MyUSDStageEditor");
	return StageEditorModule.OpenStageEditor();
}

bool UMyUsdStageEditorBlueprintLibrary::CloseStageEditor()
{
	IMyUsdStageEditorModule& StageEditorModule = FModuleManager::GetModuleChecked<IMyUsdStageEditorModule>("MyUSDStageEditor");
	return StageEditorModule.CloseStageEditor();
}

bool UMyUsdStageEditorBlueprintLibrary::IsStageEditorOpened()
{
	IMyUsdStageEditorModule& StageEditorModule = FModuleManager::GetModuleChecked<IMyUsdStageEditorModule>("MyUSDStageEditor");
	return StageEditorModule.IsStageEditorOpened();
}

AMyUsdStageActor* UMyUsdStageEditorBlueprintLibrary::GetAttachedStageActor()
{
	IMyUsdStageEditorModule& StageEditorModule = FModuleManager::GetModuleChecked<IMyUsdStageEditorModule>("MyUSDStageEditor");
	return StageEditorModule.GetAttachedStageActor();
}

bool UMyUsdStageEditorBlueprintLibrary::SetAttachedStageActor(AMyUsdStageActor* NewActor)
{
	IMyUsdStageEditorModule& StageEditorModule = FModuleManager::GetModuleChecked<IMyUsdStageEditorModule>("MyUSDStageEditor");
	return StageEditorModule.SetAttachedStageActor(NewActor);
}

TArray<FString> UMyUsdStageEditorBlueprintLibrary::GetSelectedLayerIdentifiers()
{
	IMyUsdStageEditorModule& StageEditorModule = FModuleManager::GetModuleChecked<IMyUsdStageEditorModule>("MyUSDStageEditor");
	TArray<UE::FSdfLayer> Layers = StageEditorModule.GetSelectedLayers();

	TArray<FString> LayerIdentifiers;
	LayerIdentifiers.Reserve(Layers.Num());

	for (const UE::FSdfLayer& Layer : Layers)
	{
		LayerIdentifiers.Add(Layer.GetIdentifier());
	}

	return LayerIdentifiers;
}

void UMyUsdStageEditorBlueprintLibrary::SetSelectedLayerIdentifiers(const TArray<FString>& NewSelection)
{
	TArray<UE::FSdfLayer> Layers;
	for (const FString& Identifier : NewSelection)
	{
		Layers.Add(UE::FSdfLayer::FindOrOpen(*Identifier));
	}

	IMyUsdStageEditorModule& StageEditorModule = FModuleManager::GetModuleChecked<IMyUsdStageEditorModule>("MyUSDStageEditor");
	StageEditorModule.SetSelectedLayers(Layers);
}

TArray<FString> UMyUsdStageEditorBlueprintLibrary::GetSelectedPrimPaths()
{
	IMyUsdStageEditorModule& StageEditorModule = FModuleManager::GetModuleChecked<IMyUsdStageEditorModule>("MyUSDStageEditor");
	TArray<UE::FMyUsdPrim> Prims = StageEditorModule.GetSelectedPrims();

	TArray<FString> PrimPaths;
	PrimPaths.Reserve(Prims.Num());

	for (const UE::FMyUsdPrim& Prim : Prims)
	{
		PrimPaths.Add(Prim.GetPrimPath().GetString());
	}

	return PrimPaths;
}

void UMyUsdStageEditorBlueprintLibrary::SetSelectedPrimPaths(const TArray<FString>& NewSelection)
{
	IMyUsdStageEditorModule& StageEditorModule = FModuleManager::GetModuleChecked<IMyUsdStageEditorModule>("MyUSDStageEditor");
	const AMyUsdStageActor* StageActor = StageEditorModule.GetAttachedStageActor();
	if (!StageActor)
	{
		return;
	}

	UE::FMyUsdStage Stage = StageActor->GetUsdStage();
	if (!Stage)
	{
		return;
	}

	TArray<UE::FMyUsdPrim> Prims;
	Prims.Reserve(NewSelection.Num());

	for (const FString& PrimPath : NewSelection)
	{
		if (UE::FMyUsdPrim SelectedPrim = Stage.GetPrimAtPath(UE::FSdfPath{*PrimPath}))
		{
			Prims.Add(SelectedPrim);
		}
	}

	StageEditorModule.SetSelectedPrims(Prims);
}

TArray<FString> UMyUsdStageEditorBlueprintLibrary::GetSelectedPropertyNames()
{
	IMyUsdStageEditorModule& StageEditorModule = FModuleManager::GetModuleChecked<IMyUsdStageEditorModule>("MyUSDStageEditor");
	return StageEditorModule.GetSelectedPropertyNames();
}

void UMyUsdStageEditorBlueprintLibrary::SetSelectedPropertyNames(const TArray<FString>& NewSelection)
{
	IMyUsdStageEditorModule& StageEditorModule = FModuleManager::GetModuleChecked<IMyUsdStageEditorModule>("MyUSDStageEditor");
	StageEditorModule.SetSelectedPropertyNames(NewSelection);
}

TArray<FString> UMyUsdStageEditorBlueprintLibrary::GetSelectedPropertyMetadataNames()
{
	IMyUsdStageEditorModule& StageEditorModule = FModuleManager::GetModuleChecked<IMyUsdStageEditorModule>("MyUSDStageEditor");
	return StageEditorModule.GetSelectedPropertyMetadataNames();
}

void UMyUsdStageEditorBlueprintLibrary::SetSelectedPropertyMetadataNames(const TArray<FString>& NewSelection)
{
	IMyUsdStageEditorModule& StageEditorModule = FModuleManager::GetModuleChecked<IMyUsdStageEditorModule>("MyUSDStageEditor");
	StageEditorModule.SetSelectedPropertyMetadataNames(NewSelection);
}

void UMyUsdStageEditorBlueprintLibrary::FileNew()
{
	IMyUsdStageEditorModule& StageEditorModule = FModuleManager::GetModuleChecked<IMyUsdStageEditorModule>("MyUSDStageEditor");
	StageEditorModule.FileNew();
}

void UMyUsdStageEditorBlueprintLibrary::FileOpen(const FString& FilePath)
{
	IMyUsdStageEditorModule& StageEditorModule = FModuleManager::GetModuleChecked<IMyUsdStageEditorModule>("MyUSDStageEditor");
	StageEditorModule.FileOpen(FilePath);
}

void UMyUsdStageEditorBlueprintLibrary::FileSave(const FString& OutputFilePathIfUnsaved)
{
	IMyUsdStageEditorModule& StageEditorModule = FModuleManager::GetModuleChecked<IMyUsdStageEditorModule>("MyUSDStageEditor");
	StageEditorModule.FileSave(OutputFilePathIfUnsaved);
}

void UMyUsdStageEditorBlueprintLibrary::FileExportAllLayers(const FString& OutputDirectory)
{
	IMyUsdStageEditorModule& StageEditorModule = FModuleManager::GetModuleChecked<IMyUsdStageEditorModule>("MyUSDStageEditor");
	StageEditorModule.FileExportAllLayers(OutputDirectory);
}

void UMyUsdStageEditorBlueprintLibrary::FileExportFlattenedStage(const FString& OutputLayer)
{
	IMyUsdStageEditorModule& StageEditorModule = FModuleManager::GetModuleChecked<IMyUsdStageEditorModule>("MyUSDStageEditor");
	StageEditorModule.FileExportFlattenedStage(OutputLayer);
}

void UMyUsdStageEditorBlueprintLibrary::FileExportFlattenedLayerStack(const FString& OutputLayer)
{
	IMyUsdStageEditorModule& StageEditorModule = FModuleManager::GetModuleChecked<IMyUsdStageEditorModule>("MyUSDStageEditor");
	StageEditorModule.FileExportFlattenedLayerStack(OutputLayer);
}

void UMyUsdStageEditorBlueprintLibrary::FileReload()
{
	IMyUsdStageEditorModule& StageEditorModule = FModuleManager::GetModuleChecked<IMyUsdStageEditorModule>("MyUSDStageEditor");
	StageEditorModule.FileReload();
}

void UMyUsdStageEditorBlueprintLibrary::FileReset()
{
	IMyUsdStageEditorModule& StageEditorModule = FModuleManager::GetModuleChecked<IMyUsdStageEditorModule>("MyUSDStageEditor");
	StageEditorModule.FileReset();
}

void UMyUsdStageEditorBlueprintLibrary::FileClose()
{
	IMyUsdStageEditorModule& StageEditorModule = FModuleManager::GetModuleChecked<IMyUsdStageEditorModule>("MyUSDStageEditor");
	StageEditorModule.FileClose();
}

void UMyUsdStageEditorBlueprintLibrary::ActionsImport(const FString& OutputContentFolder, UMyUsdStageImportOptions* Options)
{
	IMyUsdStageEditorModule& StageEditorModule = FModuleManager::GetModuleChecked<IMyUsdStageEditorModule>("MyUSDStageEditor");
	StageEditorModule.ActionsImport(OutputContentFolder, Options);
}

void UMyUsdStageEditorBlueprintLibrary::ExportSelectedLayers(const FString& OutputDirectory)
{
	IMyUsdStageEditorModule& StageEditorModule = FModuleManager::GetModuleChecked<IMyUsdStageEditorModule>("MyUSDStageEditor");
	StageEditorModule.ExportSelectedLayers(OutputDirectory);
}
