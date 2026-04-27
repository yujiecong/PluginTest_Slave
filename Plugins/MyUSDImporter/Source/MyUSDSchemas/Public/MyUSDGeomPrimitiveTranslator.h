// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MyUSDGeomXformableTranslator.h"

#include "MyUsdWrappers/SdfPath.h"

#define UE_API MYUSDSCHEMAS_API

#if USE_USD_SDK

class FMyUsdGeomPrimitiveTranslator : public FMyUsdGeomXformableTranslator
{
public:
	using Super = FMyUsdGeomXformableTranslator;
	using FMyUsdGeomXformableTranslator::FMyUsdGeomXformableTranslator;

	FMyUsdGeomPrimitiveTranslator(const FMyUsdGeomPrimitiveTranslator& Other) = delete;
	FMyUsdGeomPrimitiveTranslator& operator=(const FMyUsdGeomPrimitiveTranslator& Other) = delete;

	UE_API virtual void CreateAssets() override;
	UE_API virtual USceneComponent* CreateComponents() override;
	UE_API virtual void UpdateComponents(USceneComponent* SceneComponent) override;

	UE_API virtual bool CollapsesChildren(ECollapsingType CollapsingType) const override;
	UE_API virtual bool CanBeCollapsed(ECollapsingType CollapsingType) const override;

	UE_API virtual TSet<UE::FSdfPath> CollectAuxiliaryPrims() const override;
};

#endif	  // #if USE_USD_SDK

#undef UE_API
