// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#define UE_API MYUSDSTAGEIMPORTER_API

struct FMyUsdStageImportContext;

class UMyUsdStageImporter
{
public:
	UE_API void ImportFromFile(FMyUsdStageImportContext& ImportContext);

	UE_API bool ReimportSingleAsset(
		FMyUsdStageImportContext& ImportContext,
		UObject* OriginalAsset,
		const FString& OriginalPrimPath,
		UObject*& OutReimportedAsset
	);
};

#undef UE_API
