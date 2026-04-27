// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"

class UMyUsdAssetCache3;
class FMyUsdPrimLinkCache;

namespace UsdUnreal::TranslatorUtils
{
	// Properly deletes the asset and removes it from the asset and info caches, if provided
	void AbandonFailedAsset(UObject* Asset, UMyUsdAssetCache3* AssetCache, FMyUsdPrimLinkCache* PrimLinkCache);
}
