// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Objects/MyUSDSchemaTranslator.h"

#define UE_API MYUSDSCHEMAS_API

#if USE_USD_SDK

class FMyUsdShadeMaterialTranslator : public FUsdSchemaTranslator
{
public:
	using FUsdSchemaTranslator::FUsdSchemaTranslator;

	UE_API virtual void CreateAssets() override;

	UE_API virtual bool CollapsesChildren(ECollapsingType CollapsingType) const override;
	UE_API virtual bool CanBeCollapsed(ECollapsingType CollapsingType) const override;

	UE_API virtual TSet<UE::FSdfPath> CollectAuxiliaryPrims() const override;

protected:
	UE_API virtual void PostImportMaterial(const FString& PrefixedMaterialHash, UMaterialInterface* ImportedMaterial);
};

#endif	  // #if USE_USD_SDK

#undef UE_API
