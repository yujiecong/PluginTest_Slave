// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#if USE_USD_SDK && WITH_EDITOR

#include "MyUSDGeometryCacheTranslator.h"

class FMyUsdGroomTranslator : public FMyUsdGeometryCacheTranslator
{
public:
	using Super = FMyUsdGeometryCacheTranslator;
	using FMyUsdGeometryCacheTranslator::FMyUsdGeometryCacheTranslator;

	MYUSDSCHEMAS_API virtual void CreateAssets() override;

	MYUSDSCHEMAS_API virtual USceneComponent* CreateComponents() override;
	MYUSDSCHEMAS_API virtual void UpdateComponents(USceneComponent* SceneComponent) override;

	MYUSDSCHEMAS_API virtual bool CollapsesChildren(ECollapsingType CollapsingType) const override;
	MYUSDSCHEMAS_API virtual bool CanBeCollapsed(ECollapsingType CollapsingType) const override;

	MYUSDSCHEMAS_API virtual TSet<UE::FSdfPath> CollectAuxiliaryPrims() const override;

private:
	MYUSDSCHEMAS_API bool IsGroomPrim() const;
};

#endif	  // #if USE_USD_SDK
