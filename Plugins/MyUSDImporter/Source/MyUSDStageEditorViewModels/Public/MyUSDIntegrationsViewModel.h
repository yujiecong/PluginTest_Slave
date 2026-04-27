// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UsdWrappers/MyUsdAttribute.h"
#include "UsdWrappers/MyUsdStage.h"

#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"

#define UE_API MYUSDSTAGEEDITORVIEWMODELS_API

class FMyUsdIntegrationsViewModel
{
public:
	UE_API void UpdateAttributes(const UE::FMyUsdStageWeak& UsdStage, const TCHAR* PrimPath);

public:
	TArray<TSharedPtr<UE::FMyUsdAttribute>> Attributes;

	FString PrimPath;
	UE::FMyUsdStageWeak UsdStage;
};

#undef UE_API
