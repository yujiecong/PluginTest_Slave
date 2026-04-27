// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MyUSDShadeMaterialTranslator.h"

#if USE_USD_SDK

#include "USDIncludesStart.h"
#include "pxr/pxr.h"
#include "USDIncludesEnd.h"

class FMaterialXUsdShadeMaterialTranslator : public FMyUsdShadeMaterialTranslator
{
	using Super = FMyUsdShadeMaterialTranslator;

public:
	using FMyUsdShadeMaterialTranslator::FMyUsdShadeMaterialTranslator;

	MYUSDSCHEMAS_API virtual void CreateAssets() override;
};

#endif	  // #if USE_USD_SDK
