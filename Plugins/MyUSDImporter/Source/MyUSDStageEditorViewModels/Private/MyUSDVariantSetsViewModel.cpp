// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyUSDVariantSetsViewModel.h"

#include "MyUSDMemory.h"
#include "MyUSDTypesConversion.h"

#include "MyUsdWrappers/SdfPath.h"
#include "UsdWrappers/MyUsdPrim.h"

#include "ScopedTransaction.h"

#if USE_USD_SDK
#include "MyUSDIncludesStart.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/variantSets.h"
#include "MyUSDIncludesEnd.h"
#endif	  // #if USE_USD_SDK

FMyUsdVariantSetViewModel::FMyUsdVariantSetViewModel(FMyUsdVariantSetsViewModel* InOwner)
	: Owner(InOwner)
{
}

void FMyUsdVariantSetViewModel::SetVariantSelection(const TSharedPtr<FString>& InVariantSelection)
{
#if USE_USD_SDK
	if (!Owner || !Owner->UsdStage)
	{
		return;
	}

	FScopedTransaction Transaction(FText::Format(
		NSLOCTEXT("USDVariantSetsList", "SwitchVariantSetTransaction", "Switch USD Variant Set '{0}' to option '{1}'"),
		FText::FromString(SetName),
		FText::FromString(*VariantSelection)
	));

	VariantSelection = InVariantSelection;

	FScopedUsdAllocs UsdAllocs;

	std::string UsdVariantSetName = UnrealToUsd::ConvertString(*SetName).Get();
	std::string UsdVariantSelection = VariantSelection ? UnrealToUsd::ConvertString(*(*VariantSelection)).Get() : std::string{};

	if (const UE::FMyUsdPrim& Prim = Owner->UsdStage.GetPrimAtPath(UE::FSdfPath{*Owner->PrimPath}))
	{
		pxr::UsdVariantSets UsdVariantSets = pxr::UsdPrim{Prim}.GetVariantSets();
		UsdVariantSets.SetSelection(UsdVariantSetName, UsdVariantSelection);
	}

#endif	  // #if USE_USD_SDK
}

void FMyUsdVariantSetsViewModel::UpdateVariantSets(const UE::FMyUsdStageWeak& InUsdStage, const TCHAR* InPrimPath)
{
#if USE_USD_SDK
	VariantSets.Reset();
	UsdStage = InUsdStage;
	PrimPath = InPrimPath;

	if (!UsdStage || PrimPath.IsEmpty() || UE::FSdfPath{InPrimPath}.IsAbsoluteRootPath())
	{
		return;
	}

	UE::FMyUsdPrim UsdPrim = UsdStage.GetPrimAtPath(UE::FSdfPath(*PrimPath));
	if (!UsdPrim)
	{
		return;
	}

	FScopedUsdAllocs UsdAllocs;

	pxr::UsdVariantSets UsdVariantSets = pxr::UsdPrim(UsdPrim).GetVariantSets();

	std::vector<std::string> UsdVariantSetsNames;
	UsdVariantSets.GetNames(&UsdVariantSetsNames);

	for (const std::string& UsdVariantSetName : UsdVariantSetsNames)
	{
		FMyUsdVariantSetViewModel VariantSet(this);
		VariantSet.SetName = UsdToUnreal::ConvertString(UsdVariantSetName.c_str());

		pxr::UsdVariantSet UsdVariantSet = pxr::UsdPrim(UsdPrim).GetVariantSet(UsdVariantSetName.c_str());
		VariantSet.VariantSelection = MakeSharedUnreal<FString>(UsdToUnreal::ConvertString(UsdVariantSet.GetVariantSelection().c_str()));

		std::vector<std::string> VariantNames = UsdVariantSet.GetVariantNames();

		for (const std::string& VariantName : VariantNames)
		{
			VariantSet.Variants.Add(MakeSharedUnreal<FString>(UsdToUnreal::ConvertString(VariantName)));
		}

		VariantSets.Add(MakeSharedUnreal<FMyUsdVariantSetViewModel>(MoveTemp(VariantSet)));
	}
#endif	  // #if USE_USD_SDK
}
