// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyUSDStageEditorViewModelsModule.h"

#include "MyUSDMemory.h"

class FMyUsdStageEditorViewModelsModule : public IMyUsdStageEditorViewModelsModule
{
public:
};

IMPLEMENT_MODULE_USD(FMyUsdStageEditorViewModelsModule, MyUSDStageEditorViewModels);
