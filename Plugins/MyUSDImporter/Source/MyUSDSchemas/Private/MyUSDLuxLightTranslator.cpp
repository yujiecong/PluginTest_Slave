// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyUSDLuxLightTranslator.h"

#include "Objects/MyUSDPrimLinkCache.h"
#include "MyUSDAssetUserData.h"
#include "MyUSDConversionUtils.h"
#include "MyUSDDrawModeComponent.h"
#include "MyUSDErrorUtils.h"
#include "MyUSDLightConversion.h"
#include "MyUSDMemory.h"
#include "MyUSDObjectUtils.h"
#include "MyUSDShadeConversion.h"
#include "MyUSDTypesConversion.h"
#include "UsdWrappers/MyUsdPrim.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/TextureCube.h"

#if USE_USD_SDK

#include "MyUSDIncludesStart.h"
#include "pxr/usd/usdLux/diskLight.h"
#include "pxr/usd/usdLux/domeLight.h"
#include "pxr/usd/usdLux/lightAPI.h"
#include "pxr/usd/usdLux/rectLight.h"
#include "MyUSDIncludesEnd.h"

#define LOCTEXT_NAMESPACE "USDLuxLightTranslator"

void FMyUsdLuxLightTranslator::CreateAssets()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FMyUsdLuxLightTranslator::CreateAssets);

	if (!Context->UsdAssetCache || !Context->PrimLinkCache)
	{
		return;
	}

	// Don't bother generating assets if we're going to just draw some bounds for this prim instead
	EMyUsdDrawMode DrawMode = UsdUtils::GetAppliedDrawMode(GetPrim());
	if (DrawMode != EMyUsdDrawMode::Default)
	{
		CreateAlternativeDrawModeAssets(DrawMode);
		return;
	}

	const FString VolumePrimPathString = PrimPath.GetString();
	pxr::UsdPrim Prim = GetPrim();
	pxr::UsdLuxDomeLight DomeLight{Prim};
	if (!DomeLight)
	{
		// Only dome lights make assets for now
		return;
	}

	const FString ResolvedDomeTexturePath = UsdUtils::GetResolvedAssetPath(DomeLight.GetTextureFileAttr());
	if (ResolvedDomeTexturePath.IsEmpty())
	{
		FScopedUsdAllocs Allocs;

		pxr::SdfAssetPath TextureAssetPath;
		DomeLight.GetTextureFileAttr().Get<pxr::SdfAssetPath>(&TextureAssetPath);

		// Show a good warning for this because it's easy to pick some cubemap asset from the engine (that usually don't come with the
		// source texture) and have the dome light silently not work again
		FString TargetAssetPath = UsdToUnreal::ConvertString(TextureAssetPath.GetAssetPath());
		USD_LOG_USERWARNING(FText::Format(
			LOCTEXT("FailToFindTexture", "Failed to find texture '{0}' used for UsdLuxDomeLight '{1}'!"),
			FText::FromString(TargetAssetPath),
			FText::FromString(UsdToUnreal::ConvertPath(DomeLight.GetPrim().GetPath()))
		));

		return;
	}

	const FString PrefixedTextureHash = UsdUtils::GetAssetHashPrefix(Prim, Context->bShareAssetsForIdenticalPrims)
										+ LexToString(FMD5Hash::HashFile(*ResolvedDomeTexturePath));

	const FString& DesiredTextureName = FPaths::GetBaseFilename(ResolvedDomeTexturePath);

	TextureGroup Group = TextureGroup::TEXTUREGROUP_Skybox;

	bool bCreatedTexture = false;
	UTextureCube* Texture = Context->UsdAssetCache->GetOrCreateCustomCachedAsset<UTextureCube>(
		PrefixedTextureHash,
		DesiredTextureName,
		Context->ObjectFlags,
		[&ResolvedDomeTexturePath, Group](UPackage* Outer, FName SanitizedName, EObjectFlags FlagsToUse)
		{
			return UsdUtils::CreateTexture(ResolvedDomeTexturePath, SanitizedName, Group, FlagsToUse, Outer);
		},
		&bCreatedTexture
	);

	if (UMyUsdAssetUserData* TextureUserData = UsdUnreal::ObjectUtils::GetOrCreateAssetUserData(Texture))
	{
		TextureUserData->PrimPaths.AddUnique(UsdToUnreal::ConvertPath(DomeLight.GetPrim().GetPath()));
	}

	if (Texture)
	{
		Context->PrimLinkCache->LinkAssetToPrim(PrimPath, Texture);
	}
}

USceneComponent* FMyUsdLuxLightTranslator::CreateComponents()
{
	USceneComponent* SceneComponent = nullptr;

	EMyUsdDrawMode DrawMode = UsdUtils::GetAppliedDrawMode(GetPrim());
	if (DrawMode == EMyUsdDrawMode::Default)
	{
		const bool bNeedsActor = true;
		SceneComponent = CreateComponentsEx({}, bNeedsActor);
	}
	else
	{
		SceneComponent = CreateAlternativeDrawModeComponents(DrawMode);
	}

	UpdateComponents(SceneComponent);
	return SceneComponent;
}

void FMyUsdLuxLightTranslator::UpdateComponents(USceneComponent* SceneComponent)
{
	Super::UpdateComponents(SceneComponent);

	ULightComponentBase* LightComponent = Cast<ULightComponentBase>(SceneComponent);

	if (!LightComponent || !Context->PrimLinkCache)
	{
		return;
	}

	FScopedUsdAllocs UsdAllocs;

	pxr::UsdPrim Prim = GetPrim();

	const pxr::UsdLuxLightAPI LightAPI(Prim);

	if (!LightAPI)
	{
		return;
	}

	LightComponent->UnregisterComponent();

	UsdToUnreal::ConvertLight(Prim, *LightComponent, Context->Time);

	if (UDirectionalLightComponent* DirectionalLightComponent = Cast<UDirectionalLightComponent>(SceneComponent))
	{
		UsdToUnreal::ConvertDistantLight(Prim, *DirectionalLightComponent, Context->Time);
	}
	else if (URectLightComponent* RectLightComponent = Cast<URectLightComponent>(SceneComponent))
	{
		if (pxr::UsdLuxRectLight UsdRectLight{Prim})
		{
			UsdToUnreal::ConvertRectLight(Prim, *RectLightComponent, Context->Time);
		}
		else if (pxr::UsdLuxDiskLight UsdDiskLight{Prim})
		{
			UsdToUnreal::ConvertDiskLight(Prim, *RectLightComponent, Context->Time);
		}
	}
	else if (UPointLightComponent* PointLightComponent = Cast<UPointLightComponent>(SceneComponent))
	{
		if (USpotLightComponent* SpotLightComponent = Cast<USpotLightComponent>(SceneComponent))
		{
			UsdToUnreal::ConvertLuxShapingAPI(Prim, *SpotLightComponent, Context->Time);
		}

		UsdToUnreal::ConvertSphereLight(Prim, *PointLightComponent, Context->Time);
	}
	else if (USkyLightComponent* SkyLightComponent = Cast<USkyLightComponent>(SceneComponent))
	{
		SkyLightComponent->Modify();

		if (UTextureCube* TextuxeCube = Context->PrimLinkCache->GetSingleAssetForPrim<UTextureCube>(PrimPath))
		{
			SkyLightComponent->Cubemap = TextuxeCube;
			SkyLightComponent->SourceType = ESkyLightSourceType::SLS_SpecifiedCubemap;
		}

		SkyLightComponent->Mobility = EComponentMobility::Movable;	  // We won't bake geometry in the sky light so it needs to be movable
	}

	if (!LightComponent->IsRegistered())
	{
		LightComponent->RegisterComponent();
	}
}

bool FMyUsdLuxLightTranslator::CollapsesChildren(ECollapsingType CollapsingType) const
{
	// If we have a custom draw mode, it means we should draw bounds/cards/etc. instead
	// of our entire subtree, which is basically the same thing as collapsing
	EMyUsdDrawMode DrawMode = UsdUtils::GetAppliedDrawMode(GetPrim());
	if (DrawMode != EMyUsdDrawMode::Default)
	{
		return true;
	}

	return false;
}

bool FMyUsdLuxLightTranslator::CanBeCollapsed(ECollapsingType CollapsingType) const
{
	return false;
}

#undef LOCTEXT_NAMESPACE

#endif	  // #if USE_USD_SDK
