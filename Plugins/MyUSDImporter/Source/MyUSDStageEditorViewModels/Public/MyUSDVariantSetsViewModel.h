// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Templates/SharedPointer.h"

#include "UsdWrappers/MyUsdStage.h"

#define UE_API MYUSDSTAGEEDITORVIEWMODELS_API

class FMyUsdVariantSetsViewModel;

class FMyUsdVariantSetViewModel : public TSharedFromThis<FMyUsdVariantSetViewModel>
{
public:
	UE_API explicit FMyUsdVariantSetViewModel(FMyUsdVariantSetsViewModel* InOwner);

	UE_API void SetVariantSelection(const TSharedPtr<FString>& InVariantSelection);

public:
	FString SetName;
	TSharedPtr<FString> VariantSelection;
	TArray<TSharedPtr<FString>> Variants;

private:
	FMyUsdVariantSetsViewModel* Owner;
};

class FMyUsdVariantSetsViewModel
{
public:
	UE_API void UpdateVariantSets(const UE::FMyUsdStageWeak& InUsdStage, const TCHAR* PrimPath);

public:
	UE::FMyUsdStageWeak UsdStage;
	FString PrimPath;

	TArray<TSharedPtr<FMyUsdVariantSetViewModel>> VariantSets;
};

#undef UE_API
