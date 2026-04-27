// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

class UMyUsdStageImporter;

class IMyUsdStageImporterModule : public IModuleInterface
{
public:
	static inline IMyUsdStageImporterModule& Get()
	{
		return FModuleManager::LoadModuleChecked<IMyUsdStageImporterModule>("UsdStageImporter");
	}

	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("UsdStageImporter");
	}

	virtual class UMyUsdStageImporter* GetImporter() = 0;
};
