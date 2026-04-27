// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Objects/MyUSDSchemaTranslator.h"

#define UE_API MYUSDSCHEMAS_API

#if USE_USD_SDK && WITH_EDITOR

class FMyUsdNaniteAssemblyTranslator : public FUsdSchemaTranslator
{
	friend class FMyUsdNaniteAssemblyCreateAssetsTaskChain;

public:
	using Super = FUsdSchemaTranslator;

	using FUsdSchemaTranslator::FUsdSchemaTranslator;

	FMyUsdNaniteAssemblyTranslator(const FMyUsdNaniteAssemblyTranslator& Other) = delete;
	FMyUsdNaniteAssemblyTranslator& operator=(const FMyUsdNaniteAssemblyTranslator& Other) = delete;

	UE_API virtual void CreateAssets() override;
};

#endif	  // #if USE_USD_SDK && WITH_EDITOR

#undef UE_API
