#include "UEMotionMaterialManager.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionConstant.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "EditorAssetLibrary.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"

UMaterialInterface* UUEMotionMaterialManager::GetOrCreateBlackFloorMaterial()
{
	const FString ParentMaterialPath = TEXT("/Game/UEMotion/Materials/M_BlackFloorParent");
	const FString InstanceMaterialPath = TEXT("/Game/UEMotion/Materials/M_BlackFloor");

	UMaterialInterface* ParentMaterial = GetOrCreateBlackFloorParentMaterial(ParentMaterialPath);
	if (!ParentMaterial)
	{
		UE_LOG(LogTemp, Error, TEXT("UEMotionMaterialManager: Failed to create parent material"));
		return nullptr;
	}

	if (UEditorAssetLibrary::DoesAssetExist(InstanceMaterialPath))
	{
		UMaterialInterface* ExistingMat = LoadObject<UMaterialInterface>(nullptr, *InstanceMaterialPath);
		if (ExistingMat)
		{
			UE_LOG(LogTemp, Log, TEXT("UEMotionMaterialManager: Reusing existing black floor material instance '%s'"), *InstanceMaterialPath);
			CachedBlackFloorMaterial = ExistingMat;
			return ExistingMat;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("UEMotionMaterialManager: Black floor material instance exists but failed to load, recreating..."));
			UEditorAssetLibrary::DeleteAsset(InstanceMaterialPath);
		}
	}

	UPackage* InstancePackage = CreatePackage(*InstanceMaterialPath);
	if (!InstancePackage)
	{
		UE_LOG(LogTemp, Error, TEXT("UEMotionMaterialManager: Failed to create package for black floor material instance"));
		return nullptr;
	}

	UMaterialInstanceConstant* BlackMIC = NewObject<UMaterialInstanceConstant>(
		InstancePackage, TEXT("M_BlackFloor"), RF_Public | RF_Standalone | RF_Transactional);

	if (!BlackMIC)
	{
		UE_LOG(LogTemp, Error, TEXT("UEMotionMaterialManager: Failed to create black floor material instance"));
		return nullptr;
	}

	BlackMIC->SetParentEditorOnly(ParentMaterial);

#if WITH_EDITOR
	FLinearColor PureBlack(0.0f, 0.0f, 0.0f, 1.0f);
	BlackMIC->SetVectorParameterValueEditorOnly(FName("BaseColor"), PureBlack);
	BlackMIC->PostEditChange();
#endif

	BlackMIC->SetFlags(RF_Public | RF_Standalone);
	InstancePackage->MarkPackageDirty();

	FString InstanceFilePath = FPaths::Combine(
		FPaths::ProjectContentDir(),
		TEXT("UEMotion/Materials/"),
		TEXT("M_BlackFloor.uasset"));

	FSavePackageArgs SaveArgs;
	SaveArgs.SaveFlags = RF_Public | RF_Standalone;

	bool bSaveSuccess = UPackage::SavePackage(
		InstancePackage,
		BlackMIC,
		*InstanceFilePath,
		SaveArgs);

	if (bSaveSuccess)
	{
		FAssetRegistryModule::AssetCreated(BlackMIC);

		UE_LOG(LogTemp, Log,
			TEXT("UEMotionMaterialManager: Created black floor material INSTANCE UAsset '%s'")
			TEXT("\n  Parent: M_BlackFloorParent (Custom Unlit Material)")
			TEXT("\n  BaseColor: (0, 0, 0, 1) [Pure Black]")
			TEXT("\n  Type: MaterialInstanceConstant (Static UAsset)")
			TEXT("\n  ✓ Both parent and instance saved as .uasset files"),
			*InstanceFilePath);

#if WITH_EDITOR
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		TArray<FString> MaterialPaths;
		MaterialPaths.Add(InstanceMaterialPath);
		AssetRegistryModule.Get().ScanPathsSynchronous(MaterialPaths);

		UE_LOG(LogTemp, Log, TEXT("UEMotionMaterialManager: Force synced asset registry for '%s'"), *InstanceMaterialPath);
#endif

		CachedBlackFloorMaterial = BlackMIC;
		return BlackMIC;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("UEMotionMaterialManager: Failed to save black floor material instance to '%s'"), *InstanceFilePath);
		return nullptr;
	}
}

UMaterialInterface* UUEMotionMaterialManager::GetOrCreateBlackFloorParentMaterial(const FString& ParentMaterialPath)
{
	if (CachedBlackFloorParentMaterial)
	{
		return CachedBlackFloorParentMaterial;
	}

	if (UEditorAssetLibrary::DoesAssetExist(ParentMaterialPath))
	{
		UMaterialInterface* ExistingMat = LoadObject<UMaterialInterface>(nullptr, *ParentMaterialPath);
		if (ExistingMat)
		{
			UE_LOG(LogTemp, Log, TEXT("UEMotionMaterialManager: Reusing existing black floor PARENT material '%s'"), *ParentMaterialPath);
			CachedBlackFloorParentMaterial = ExistingMat;
			return ExistingMat;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("UEMotionMaterialManager: Parent material exists but failed to load, recreating..."));
			UEditorAssetLibrary::DeleteAsset(ParentMaterialPath);
		}
	}

	UPackage* ParentPackage = CreatePackage(*ParentMaterialPath);
	if (!ParentPackage)
	{
		UE_LOG(LogTemp, Error, TEXT("UEMotionMaterialManager: Failed to create package for black floor parent material"));
		return nullptr;
	}

	UMaterial* ParentMaterial = NewObject<UMaterial>(
		ParentPackage, TEXT("M_BlackFloorParent"), RF_Public | RF_Standalone | RF_Transactional);

	if (!ParentMaterial)
	{
		UE_LOG(LogTemp, Error, TEXT("UEMotionMaterialManager: Failed to create black floor parent material"));
		return nullptr;
	}

	ParentMaterial->MaterialDomain = EMaterialDomain::MD_Surface;
	ParentMaterial->BlendMode = EBlendMode::BLEND_Opaque;
	ParentMaterial->TwoSided = true;

	UMaterialExpressionVectorParameter* BaseColorParam = NewObject<UMaterialExpressionVectorParameter>(ParentMaterial);
	BaseColorParam->ParameterName = FName("BaseColor");
	BaseColorParam->DefaultValue = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);
	ParentMaterial->GetExpressionCollection().AddExpression(BaseColorParam);

	UMaterialExpressionScalarParameter* MetallicParam = NewObject<UMaterialExpressionScalarParameter>(ParentMaterial);
	MetallicParam->ParameterName = FName("Metallic");
	MetallicParam->DefaultValue = 0.0f;
	ParentMaterial->GetExpressionCollection().AddExpression(MetallicParam);

	UMaterialExpressionScalarParameter* SpecularParam = NewObject<UMaterialExpressionScalarParameter>(ParentMaterial);
	SpecularParam->ParameterName = FName("Specular");
	SpecularParam->DefaultValue = 0.0f;
	ParentMaterial->GetExpressionCollection().AddExpression(SpecularParam);

	UMaterialExpressionScalarParameter* RoughnessParam = NewObject<UMaterialExpressionScalarParameter>(ParentMaterial);
	RoughnessParam->ParameterName = FName("Roughness");
	RoughnessParam->DefaultValue = 1.0f;
	ParentMaterial->GetExpressionCollection().AddExpression(RoughnessParam);

	UMaterialExpressionVectorParameter* EmissiveParam = NewObject<UMaterialExpressionVectorParameter>(ParentMaterial);
	EmissiveParam->ParameterName = FName("EmissiveColor");
	EmissiveParam->DefaultValue = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);
	ParentMaterial->GetExpressionCollection().AddExpression(EmissiveParam);

#if WITH_EDITORONLY_DATA
	UMaterialEditorOnlyData* EditorData = ParentMaterial->GetEditorOnlyData();
	if (EditorData)
	{
		EditorData->BaseColor.Expression = BaseColorParam;
		EditorData->BaseColor.OutputIndex = 0;
		EditorData->Metallic.Expression = MetallicParam;
		EditorData->Metallic.OutputIndex = 0;
		EditorData->Specular.Expression = SpecularParam;
		EditorData->Specular.OutputIndex = 0;
		EditorData->Roughness.Expression = RoughnessParam;
		EditorData->Roughness.OutputIndex = 0;
		EditorData->EmissiveColor.Expression = EmissiveParam;
		EditorData->EmissiveColor.OutputIndex = 0;
	}
#endif

	ParentMaterial->SetFlags(RF_Public | RF_Standalone);
	ParentPackage->MarkPackageDirty();

#if WITH_EDITOR
	ParentMaterial->PostEditChange();
#endif

	FString ParentFilePath = FPaths::Combine(
		FPaths::ProjectContentDir(),
		TEXT("UEMotion/Materials/"),
		TEXT("M_BlackFloorParent.uasset"));

	FSavePackageArgs SaveArgs;
	SaveArgs.SaveFlags = RF_Public | RF_Standalone;

	bool bSaveSuccess = UPackage::SavePackage(
		ParentPackage,
		ParentMaterial,
		*ParentFilePath,
		SaveArgs);

	if (bSaveSuccess)
	{
		FAssetRegistryModule::AssetCreated(ParentMaterial);

		UE_LOG(LogTemp, Log,
			TEXT("UEMotionMaterialManager: Created VANTABLACK PARENT material UAsset '%s'")
			TEXT("\n  [PBR] BaseColor=(0,0,0) Specular=0 Roughness=1 Metallic=0 Emissive=(0,0,0)")
			TEXT("\n  [Result] Absorbs all light (Zero reflection)"),
			*ParentFilePath);

#if WITH_EDITOR
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		TArray<FString> MaterialPaths;
		MaterialPaths.Add(ParentMaterialPath);
		AssetRegistryModule.Get().ScanPathsSynchronous(MaterialPaths);

		UE_LOG(LogTemp, Log, TEXT("UEMotionMaterialManager: Force synced asset registry for '%s'"), *ParentMaterialPath);
#endif

		CachedBlackFloorParentMaterial = ParentMaterial;
		return ParentMaterial;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("UEMotionMaterialManager: Failed to save black floor parent material to '%s'"), *ParentFilePath);
		return nullptr;
	}
}

UMaterialInterface* UUEMotionMaterialManager::EnsureBaseTranslucentMaterial(const FString& MaterialPath)
{
	if (CachedBaseTranslucentMaterial)
	{
		return CachedBaseTranslucentMaterial;
	}

	UPackage* Package = CreatePackage(*MaterialPath);
	if (!Package) return nullptr;

	UMaterial* Material = NewObject<UMaterial>(Package, FName(TEXT("M_UEMotionBaseTranslucent")), RF_Public | RF_Standalone);
	if (!Material) return nullptr;

	Material->BlendMode = EBlendMode::BLEND_Translucent;
	Material->TwoSided = 1;

	UMaterialExpressionVectorParameter* ColorExpr = NewObject<UMaterialExpressionVectorParameter>(Material);
	ColorExpr->ParameterName = FName("BaseColor");
	ColorExpr->DefaultValue = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);
	Material->GetExpressionCollection().AddExpression(ColorExpr);

	UMaterialExpressionScalarParameter* OpacityParam = NewObject<UMaterialExpressionScalarParameter>(Material);
	OpacityParam->ParameterName = FName("Opacity");
	OpacityParam->DefaultValue = 1.0f;
	Material->GetExpressionCollection().AddExpression(OpacityParam);

	UMaterialExpressionConstant* RoughnessConst = NewObject<UMaterialExpressionConstant>(Material);
	RoughnessConst->R = 0.5f;
	Material->GetExpressionCollection().AddExpression(RoughnessConst);

	UMaterialExpressionConstant* MetallicConst = NewObject<UMaterialExpressionConstant>(Material);
	MetallicConst->R = 0.0f;
	Material->GetExpressionCollection().AddExpression(MetallicConst);

	UMaterialEditorOnlyData* EditorData = Material->GetEditorOnlyData();
	if (EditorData)
	{
		EditorData->BaseColor.Expression = ColorExpr;
		EditorData->BaseColor.OutputIndex = 0;
		EditorData->Opacity.Expression = OpacityParam;
		EditorData->Opacity.OutputIndex = 0;
		EditorData->Roughness.Expression = RoughnessConst;
		EditorData->Roughness.OutputIndex = 0;
		EditorData->Metallic.Expression = MetallicConst;
		EditorData->Metallic.OutputIndex = 0;
	}

	SaveAssetToObject(Material, TEXT("/Game/UEMotion/Materials"), TEXT("M_UEMotionBaseTranslucent"));

	FAssetRegistryModule::AssetCreated(Material);
	Material->MarkPackageDirty();
#if WITH_EDITOR
	Material->PostEditChange();
#endif

	UE_LOG(LogTemp, Log, TEXT("UEMotionMaterialManager: Created translucent material '%s'"), *MaterialPath);

	CachedBaseTranslucentMaterial = Material;
	return Material;
}

UMaterialInterface* UUEMotionMaterialManager::GetOrCreateBaseMaterial()
{
	if (CachedBaseMaterial) return CachedBaseMaterial;

	UMaterialInterface* TranslucentMat = EnsureBaseTranslucentMaterial();
	if (TranslucentMat)
	{
		CachedBaseMaterial = TranslucentMat;
		return CachedBaseMaterial;
	}

	CachedBaseMaterial = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	return CachedBaseMaterial;
}

bool UUEMotionMaterialManager::SaveAssetToObject(UObject* Asset, const FString& PackagePath, const FString& AssetName)
{
	if (!Asset) return false;

	UPackage* Package = Cast<UPackage>(Asset->GetOuter());
	if (!Package)
	{
		Package = Asset->GetOutermost();
	}
	if (!Package) return false;

	Asset->SetFlags(RF_Public | RF_Standalone);
	Package->MarkPackageDirty();

#if WITH_EDITOR
	Asset->PostEditChange();
	Package->MarkPackageDirty();
#endif

	FString PackageName = Package->GetName();
	FString Filename = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;

	bool bSaved = UPackage::SavePackage(Package, Asset, *Filename, SaveArgs);
	if (bSaved)
	{
		FAssetRegistryModule::AssetCreated(Asset);
		FString FullAssetPath = FString::Printf(TEXT("%s/%s"), *PackagePath, *AssetName);
		UE_LOG(LogTemp, Log, TEXT("UEMotionMaterialManager: Saved asset '%s'"), *FullAssetPath);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UEMotionMaterialManager: Failed to save asset '%s'"), *AssetName);
	}

	return bSaved;
}
