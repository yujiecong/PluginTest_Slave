// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyUSDSchemasModule.h"

#define private public
#include "Objects/USDSchemaTranslator.h"
#undef private
#include "USDMemory.h"
#include "USDTypesConversion.h"

#include "AnalyticsEventAttribute.h"
#include "Modules/ModuleManager.h"

#if USE_USD_SDK
#include "Custom/MaterialXUSDShadeMaterialTranslator.h"
#include "MyUSDGeomCameraTranslator.h"
#include "MyUSDGeometryCacheTranslator.h"
#include "MyUSDGeomMeshTranslator.h"
#include "MyUSDGeomPointInstancerTranslator.h"
#include "MyUSDGeomPrimitiveTranslator.h"
#include "MyUSDGeomXformableTranslator.h"
#include "MyUSDGroomTranslator.h"
#include "MyUSDLuxLightTranslator.h"
#include "MyUSDMediaSpatialAudioTranslator.h"
#include "USDShadeConversion.h"
#include "MyUSDShadeMaterialTranslator.h"
#include "MyUSDSkelSkeletonTranslator.h"
#include "MyUSDVolVolumeTranslator.h"

#include "USDIncludesStart.h"
#include "pxr/usd/kind/registry.h"
#include "pxr/usd/usd/primRange.h"
#include "USDIncludesEnd.h"
#endif	  // #if USE_USD_SDK

class FMyUsdSchemasModule : public IMyUsdSchemasModule
{
public:
	virtual void StartupModule() override
	{
#if USE_USD_SDK
		LLM_SCOPE_BYTAG(Usd);

		FUsdSchemaTranslatorRegistry& Registry = FUsdSchemaTranslatorRegistry::Get();

		// Register the default translators
		TranslatorHandles = {
			Registry.Register<FMyUsdGeomCameraTranslator>(TEXT("UsdGeomCamera")),
			Registry.Register<FMyUsdGeomMeshTranslator>(TEXT("UsdGeomMesh")),
			Registry.Register<FMyUsdGeomPrimitiveTranslator>(TEXT("UsdGeomCapsule")),
			Registry.Register<FMyUsdGeomPrimitiveTranslator>(TEXT("UsdGeomCone")),
			Registry.Register<FMyUsdGeomPrimitiveTranslator>(TEXT("UsdGeomCube")),
			Registry.Register<FMyUsdGeomPrimitiveTranslator>(TEXT("UsdGeomCylinder")),
			Registry.Register<FMyUsdGeomPrimitiveTranslator>(TEXT("UsdGeomPlane")),
			Registry.Register<FMyUsdGeomPrimitiveTranslator>(TEXT("UsdGeomSphere")),
			Registry.Register<FMyUsdGeomPointInstancerTranslator>(TEXT("UsdGeomPointInstancer")),
			Registry.Register<FMyUsdGeomXformableTranslator>(TEXT("UsdGeomXformable")),
			Registry.Register<FMyUsdShadeMaterialTranslator>(TEXT("UsdShadeMaterial")),
			Registry.Register<FMyUsdLuxLightTranslator>(TEXT("UsdLuxBoundableLightBase")),
			Registry.Register<FMyUsdLuxLightTranslator>(TEXT("UsdLuxNonboundableLightBase")),
			Registry.Register<FMyUsdVolVolumeTranslator>(TEXT("UsdVolVolume"))
		};

#if WITH_EDITOR
		UsdUnreal::MaterialUtils::RegisterRenderContext(UnrealIdentifiers::MaterialXRenderContext);
		TranslatorHandles.Add(Registry.Register<FMaterialXUsdShadeMaterialTranslator>(TEXT("UsdShadeMaterial")));

		// Creating skeletal meshes technically works in Standalone mode, but by checking for this we artificially block it
		// to not confuse users as to why it doesn't work at runtime. Not registering the actual translators lets the inner meshes get parsed as
		// static meshes, at least.
		if (GIsEditor)
		{
			TranslatorHandles.Append(
				{Registry.Register<FMyUsdSkelSkeletonTranslator>(TEXT("UsdSkelSkeleton")),
				 Registry.Register<FMyUsdGroomTranslator>(TEXT("UsdGeomXformable")),
				 // The GeometryCacheTranslator also works on UsdGeomXformable through the GroomTranslator
				 Registry.Register<FMyUsdGeometryCacheTranslator>(TEXT("UsdGeomMesh")),
				 // It doesn't seem possible to create SoundWave assets at runtime at the moment, for whatever reason
				 Registry.Register<FMyUsdMediaSpatialAudioTranslator>(TEXT("UsdMediaSpatialAudio"))}
			);
		}
#endif	  // WITH_EDITOR

		Registry.ResetExternalTranslatorCount();

#endif	  // #if USE_USD_SDK
	}

	virtual void ShutdownModule() override
	{
		FUsdSchemaTranslatorRegistry& Registry = FUsdSchemaTranslatorRegistry::Get();

		for (const FRegisteredSchemaTranslatorHandle& TranslatorHandle : TranslatorHandles)
		{
			Registry.Unregister(TranslatorHandle);
		}

#if USE_USD_SDK && WITH_EDITOR
		UsdUnreal::MaterialUtils::UnregisterRenderContext(UnrealIdentifiers::MaterialXRenderContext);
#endif	  // WITH_EDITOR
	}

	virtual FUsdSchemaTranslatorRegistry& GetTranslatorRegistry() override
	{
		return FUsdSchemaTranslatorRegistry::Get();
	}

	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	virtual FUsdRenderContextRegistry& GetRenderContextRegistry() override
	{
		return UsdRenderContextRegistry;
	}
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

protected:
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	FUsdRenderContextRegistry UsdRenderContextRegistry;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

	TArray<FRegisteredSchemaTranslatorHandle> TranslatorHandles;
};

void UsdUnreal::Analytics::CollectSchemaAnalytics(const UE::FUsdStage& Stage, const FString& EventName)
{
#if USE_USD_SDK
	if (!Stage)
	{
		return;
	}

	// Last collected at cl 32482283
	const static TSet<FString> NativeSchemaNames{
		"AssetPreviewsAPI",
		"Backdrop",
		"BasisCurves",
		"BlendShape",
		"Camera",
		"Capsule",
		"Capsule_1",
		"ClipsAPI",
		"CollectionAPI",
		"Cone",
		"ConnectableAPI",
		"ControlRigAPI",
		"CoordSysAPI",
		"Cube",
		"Cylinder",
		"CylinderLight",
		"Cylinder_1",
		"DiskLight",
		"DistantLight",
		"DomeLight",
		"DomeLight_1",
		"Field3DAsset",
		"GenerativeProcedural",
		"GeomModelAPI",
		"GeomSubset",
		"GeometryLight",
		"GroomAPI",
		"GroomBindingAPI",
		"HermiteCurves",
		"HydraGenerativeProceduralAPI",
		"LightAPI",
		"LightFilter",
		"LightListAPI",
		"ListAPI",
		"LiveLinkAPI",
		"Material",
		"MaterialBindingAPI",
		"Mesh",
		"MeshLightAPI",
		"ModelAPI",
		"MotionAPI",
		"NodeDefAPI",
		"NodeGraph",
		"NodeGraphNodeAPI",
		"NurbsCurves",
		"NurbsPatch",
		"OpenVDBAsset",
		"PhysicsArticulationRootAPI",
		"PhysicsCollisionAPI",
		"PhysicsCollisionGroup",
		"PhysicsDistanceJoint",
		"PhysicsDriveAPI",
		"PhysicsFilteredPairsAPI",
		"PhysicsFixedJoint",
		"PhysicsJoint",
		"PhysicsLimitAPI",
		"PhysicsMassAPI",
		"PhysicsMaterialAPI",
		"PhysicsMeshCollisionAPI",
		"PhysicsPrismaticJoint",
		"PhysicsRevoluteJoint",
		"PhysicsRigidBodyAPI",
		"PhysicsScene",
		"PhysicsSphericalJoint",
		"Plane",
		"PluginLight",
		"PluginLightFilter",
		"PointInstancer",
		"Points",
		"PortalLight",
		"PrimvarsAPI",
		"RectLight",
		"RenderDenoisePass",
		"RenderPass",
		"RenderProduct",
		"RenderSettings",
		"RenderVar",
		"RiMaterialAPI",
		"RiRenderPassAPI",
		"RiSplineAPI",
		"SceneGraphPrimAPI",
		"Scope",
		"Shader",
		"ShadowAPI",
		"ShapingAPI",
		"SkelAnimation",
		"SkelBindingAPI",
		"SkelRoot",
		"Skeleton",
		"SparseVolumeTextureAPI",
		"SpatialAudio",
		"Sphere",
		"SphereLight",
		"StatementsAPI",
		"TetMesh",
		"VisibilityAPI",
		"Volume",
		"VolumeLightAPI",
		"Xform",
		"XformCommonAPI"
	};

	// clang-format off
	// "Interesting" here means either something we don't support, or something that would be interesting
	// to check the usage of, like for our Groom/ControlRig/LiveLink support, or whether the new support
	// for OpenVDB is used, etc.
	TMap<FString, int32> InterestingNativeSchemaCounts{
		{"AssetPreviewsAPI", 0},
		{"Backdrop", 0},
		{"BasisCurves", 0},
		{"Capsule_1", 0},
		{"ClipsAPI", 0},
		{"CollectionAPI", 0},
		{"ControlRigAPI", 0},
		{"CoordSysAPI", 0},
		{"CylinderLight", 0},
		{"Cylinder_1", 0},
		{"DomeLight_1", 0},
		{"Field3DAsset", 0},
		{"GenerativeProcedural", 0},
		{"GeometryLight", 0},
		{"GroomAPI", 0},
		{"GroomBindingAPI", 0},
		{"HermiteCurves", 0},
		{"HydraGenerativeProceduralAPI", 0},
		{"LightFilter", 0},
		{"LightListAPI", 0},
		{"ListAPI", 0},
		{"LiveLinkAPI", 0},
		{"MeshLightAPI", 0},
		{"MotionAPI", 0},
		{"NurbsCurves", 0},
		{"NurbsPatch", 0},
		{"OpenVDBAsset", 0},
		{"PhysicsArticulationRootAPI", 0},
		{"PhysicsCollisionAPI", 0},
		{"PhysicsCollisionGroup", 0},
		{"PhysicsDistanceJoint", 0},
		{"PhysicsDriveAPI", 0},
		{"PhysicsFilteredPairsAPI", 0},
		{"PhysicsFixedJoint", 0},
		{"PhysicsJoint", 0},
		{"PhysicsLimitAPI", 0},
		{"PhysicsMassAPI", 0},
		{"PhysicsMaterialAPI", 0},
		{"PhysicsMeshCollisionAPI", 0},
		{"PhysicsPrismaticJoint", 0},
		{"PhysicsRevoluteJoint", 0},
		{"PhysicsRigidBodyAPI", 0},
		{"PhysicsScene", 0},
		{"PhysicsSphericalJoint", 0},
		{"PluginLight", 0},
		{"PluginLightFilter", 0},
		{"Points", 0},
		{"PortalLight", 0},
		{"RenderDenoisePass", 0},
		{"RenderPass", 0},
		{"RenderProduct", 0},
		{"RenderSettings", 0},
		{"RenderVar", 0},
		{"RiMaterialAPI", 0},
		{"RiRenderPassAPI", 0},
		{"RiSplineAPI", 0},
		{"SceneGraphPrimAPI", 0},
		{"ShadowAPI", 0},
		{"SparseVolumeTextureAPI", 0},
		{"SpatialAudio", 0},
		{"SphereLight", 0},
		{"StatementsAPI", 0},
		{"TetMesh", 0},
		{"VisibilityAPI", 0},
		{"Volume", 0},
		{"VolumeLightAPI", 0}
	};
	// clang-format on

	TSet<FString> SeenSchemas;

	int32 CustomSchemaCount = 0;
	{
		FScopedUsdAllocs Allocs;

		pxr::UsdPrimRange PrimRange{Stage.GetPseudoRoot(), pxr::UsdTraverseInstanceProxies()};
		for (pxr::UsdPrimRange::iterator PrimRangeIt = ++PrimRange.begin(); PrimRangeIt != PrimRange.end(); ++PrimRangeIt)
		{
			// It's perfectly fine to have a typeless prim (e.g. "def 'Cube'")
			if (PrimRangeIt->HasAuthoredTypeName())
			{
				const pxr::TfToken& TypeName = PrimRangeIt->GetTypeName();
				const FString TypeNameStr = UsdToUnreal::ConvertToken(TypeName);

				if (!TypeNameStr.IsEmpty())
				{
					SeenSchemas.Add(TypeNameStr);

					if (int32* InterestingSchemaCount = InterestingNativeSchemaCounts.Find(TypeNameStr))
					{
						*InterestingSchemaCount = *InterestingSchemaCount + 1;
					}
				}
			}

			const pxr::UsdPrimTypeInfo& PrimTypeInfo = PrimRangeIt->GetPrimTypeInfo();
			for (const pxr::TfToken& AppliedSchema : PrimTypeInfo.GetAppliedAPISchemas())
			{
				std::pair<pxr::TfToken, pxr::TfToken> Pair = pxr::UsdSchemaRegistry::GetTypeNameAndInstance(AppliedSchema);
				FString TypeName = UsdToUnreal::ConvertToken(Pair.first);

				// These applied schema names shouldn't ever end up as the empty string... but we don't really want to pop
				// an ensure or show a warning when analytics fails
				if (!TypeName.IsEmpty())
				{
					SeenSchemas.Add(TypeName);

					if (int32* InterestingSchemaCount = InterestingNativeSchemaCounts.Find(TypeName))
					{
						*InterestingSchemaCount = *InterestingSchemaCount + 1;
					}
				}
			}
		}
	}

	for (const FString& SchemaName : SeenSchemas)
	{
		// We only care about non-native schemas
		if (!NativeSchemaNames.Contains(SchemaName))
		{
			++CustomSchemaCount;
		}
	}

	TArray<FAnalyticsEventAttribute> EventAttributes;
	if (CustomSchemaCount > 0)
	{
		EventAttributes.Emplace(TEXT("CustomSchemas"), CustomSchemaCount);
	}

	for (const TPair<FString, int32>& Pair : InterestingNativeSchemaCounts)
	{
		const FString& InterestingNativeSchemaName = Pair.Key;
		int32 Count = Pair.Value;
		if (Count > 0)
		{
			EventAttributes.Emplace(InterestingNativeSchemaName, Count);
		}
	}

	FUsdSchemaTranslatorRegistry& Registry = FUsdSchemaTranslatorRegistry::Get();
	int32 SchemaTranslatorCount = Registry.GetExternalSchemaTranslatorCount();
	if (SchemaTranslatorCount > 0)
	{
		EventAttributes.Emplace(TEXT("CustomSchemaTranslatorCount"), SchemaTranslatorCount);
	}

	if (EventAttributes.Num() > 0)
	{
		IUsdClassesModule::SendAnalytics(MoveTemp(EventAttributes), FString::Printf(TEXT("%s.CustomSchemaCount"), *EventName));
	}
#endif	  // USE_USD_SDK
}

IMPLEMENT_MODULE_USD(FMyUsdSchemasModule, MyUSDSchemas);
