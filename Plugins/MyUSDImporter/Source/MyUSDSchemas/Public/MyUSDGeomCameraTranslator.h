// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MyUSDGeomXformableTranslator.h"

#define UE_API MYUSDSCHEMAS_API

#if USE_USD_SDK

class FMyUsdGeomCameraTranslator : public FMyUsdGeomXformableTranslator
{
public:
	using FMyUsdGeomXformableTranslator::FMyUsdGeomXformableTranslator;

	UE_API virtual USceneComponent* CreateComponents() override;
	UE_API virtual void UpdateComponents(USceneComponent* SceneComponent) override;

	UE_API virtual bool CollapsesChildren(ECollapsingType CollapsingType) const override;
	UE_API virtual bool CanBeCollapsed(ECollapsingType CollapsingType) const override;
};

#endif	  // #if USE_USD_SDK

#undef UE_API
