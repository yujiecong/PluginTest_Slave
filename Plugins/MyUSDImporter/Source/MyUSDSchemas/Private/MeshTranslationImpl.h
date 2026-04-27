// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#if USE_USD_SDK

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"

#include "MyUSDIncludesStart.h"
#include "pxr/pxr.h"
#include "MyUSDIncludesEnd.h"

PXR_NAMESPACE_OPEN_SCOPE
	class UsdPrim;
PXR_NAMESPACE_CLOSE_SCOPE

class FMyUsdInfoCache;
class FMyUsdPrimLinkCache;
class UMaterialInterface;
class UMeshComponent;
class UMyUsdAssetCache3;
class UMyUsdMeshAssetUserData;
struct FMyUsdSchemaTranslationContext;
namespace UsdUtils
{
	struct FMyUsdPrimMaterialSlot;
	struct FMyUsdPrimMaterialAssignmentInfo;
}

/** Implementation that can be shared between the Skeleton translator and GeomMesh translators */
namespace MeshTranslationImpl
{
	/** Resolves the material assignments in AssignmentInfo, returning an UMaterialInterface for each material slot */
	TMap<const UsdUtils::FMyUsdPrimMaterialSlot*, UMaterialInterface*> ResolveMaterialAssignmentInfo(
		const pxr::UsdPrim& UsdPrim,
		const TArray<UsdUtils::FMyUsdPrimMaterialAssignmentInfo>& AssignmentInfo,
		UMyUsdAssetCache3& AssetCache,
		FMyUsdPrimLinkCache& PrimLinkCache,
		EObjectFlags Flags,
		bool bShareAssetsForIdenticalPrims
	);

	/**
	 * Sets the material overrides on MeshComponent according to the material assignments of the UsdGeomMesh Prim.
	 * Warning: This function will temporarily switch the active LOD variant if one exists, so it's *not* thread safe!
	 */
	void SetMaterialOverrides(
		const pxr::UsdPrim& Prim,
		const TArray<UMaterialInterface*>& ExistingAssignments,
		UMeshComponent& MeshComponent,
		FMyUsdSchemaTranslationContext& Context
	);

	void RecordSourcePrimsForMaterialSlots(
		const TArray<UsdUtils::FMyUsdPrimMaterialAssignmentInfo>& LODIndexToMaterialInfo,
		UMyUsdMeshAssetUserData* UserData
	);
}	 // namespace MeshTranslationImpl

#endif	  // #if USE_USD_SDK
