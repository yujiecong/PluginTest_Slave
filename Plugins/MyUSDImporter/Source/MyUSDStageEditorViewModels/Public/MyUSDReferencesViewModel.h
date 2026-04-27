// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Templates/SharedPointer.h"

#include "MyUsdWrappers/ForwardDeclarations.h"
#include "MyUsdWrappers/SdfPath.h"
#include "UsdWrappers/MyUsdStage.h"

#define UE_API MYUSDSTAGEEDITORVIEWMODELS_API

class FMyUsdReference : public TSharedFromThis<FMyUsdReference>
{
public:
	FString AssetPath;
	FString PrimPath;
	bool bIntroducedInLocalLayerStack = true;
	bool bIsPayload = false;
};

class FMyUsdReferencesViewModel
{
public:
	UE_API void UpdateReferences(const UE::FUsdStageWeak& UsdStage, const TCHAR* PrimPath);
	UE_API void RemoveReference(const TSharedPtr<FMyUsdReference>& Reference);
	UE_API void ReloadReference(const TSharedPtr<FMyUsdReference>& Reference);

public:
	UE::FUsdStageWeak UsdStage;
	UE::FSdfPath PrimPath;
	TArray<TSharedPtr<FMyUsdReference>> References;
};

#undef UE_API
