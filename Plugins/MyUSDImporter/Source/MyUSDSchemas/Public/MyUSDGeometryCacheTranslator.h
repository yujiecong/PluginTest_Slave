// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#if USE_USD_SDK && WITH_EDITOR

#include "MyUSDGeomMeshTranslator.h"

class FMyUsdGeometryCacheTranslator : public FMyUsdGeomMeshTranslator
{
public:
	using Super = FMyUsdGeomMeshTranslator;

	using FMyUsdGeomMeshTranslator::FMyUsdGeomMeshTranslator;

	FMyUsdGeometryCacheTranslator(const FMyUsdGeometryCacheTranslator& Other) = delete;
	FMyUsdGeometryCacheTranslator& operator=(const FMyUsdGeometryCacheTranslator& Other) = delete;

	MYUSDSCHEMAS_API virtual void CreateAssets() override;
	MYUSDSCHEMAS_API virtual USceneComponent* CreateComponents() override;
	MYUSDSCHEMAS_API virtual void UpdateComponents(USceneComponent* SceneComponent) override;

	MYUSDSCHEMAS_API virtual bool CollapsesChildren(ECollapsingType CollapsingType) const override;
	MYUSDSCHEMAS_API virtual bool CanBeCollapsed(ECollapsingType CollapsingType) const override;

	MYUSDSCHEMAS_API virtual TSet<UE::FSdfPath> CollectAuxiliaryPrims() const override;

private:
	MYUSDSCHEMAS_API bool IsPotentialGeometryCacheRoot() const;
};

#endif	  // #if USE_USD_SDK
