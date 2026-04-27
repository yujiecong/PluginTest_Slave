// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MyUSDGeomMeshTranslator.h"

#if USE_USD_SDK

class MYUSDSCHEMAS_API UE_DEPRECATED(5.4, "Use the UsdSkelSkeletonTranslator for skeletal data") FUsdSkelRootTranslator
	: public FMyUsdGeomXformableTranslator
{
	using Super = FMyUsdGeomXformableTranslator;

public:
	using FMyUsdGeomXformableTranslator::FMyUsdGeomXformableTranslator;

	virtual void CreateAssets() override;
	virtual USceneComponent* CreateComponents() override;
	virtual void UpdateComponents(USceneComponent* SceneComponent) override;

	virtual bool CollapsesChildren(ECollapsingType CollapsingType) const override;
	virtual bool CanBeCollapsed(ECollapsingType CollapsingType) const override;

	virtual TSet<UE::FSdfPath> CollectAuxiliaryPrims() const override;
};

#endif	  // #if USE_USD_SDK
