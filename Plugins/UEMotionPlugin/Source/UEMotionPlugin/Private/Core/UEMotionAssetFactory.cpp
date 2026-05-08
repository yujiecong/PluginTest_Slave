#include "UEMotionAssetFactory.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionConstant.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "EditorAssetLibrary.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "FileHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Blueprint.h"
#include "Factories/BlueprintFactory.h"

FString UUEMotionAssetFactory::ResolveAssetPath(const FString& PackagePath, const FString& AssetName) const
{
	return FString::Printf(TEXT("%s/%s"), *PackagePath, *AssetName);
}

UMaterialInterface* UUEMotionAssetFactory::GetOrCreateBaseMaterial()
{
	if (CachedBaseMaterial) return CachedBaseMaterial;

	UMaterial* Material = NewObject<UMaterial>(GetTransientPackage(), TEXT("M_UEMotionAssetBase"), RF_Transient | RF_Public);
	if (!Material)
	{
		CachedBaseMaterial = LoadObject<UMaterialInterface>(
			nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		return CachedBaseMaterial;
	}

	Material->BlendMode = EBlendMode::BLEND_Translucent;
	Material->TwoSided = 1;

	UMaterialExpressionVectorParameter* ColorExpr = NewObject<UMaterialExpressionVectorParameter>(Material);
	ColorExpr->ParameterName = FName("BaseColor");
	ColorExpr->DefaultValue = FLinearColor(0.5f, 0.5f, 0.5f, 1.0f);
	Material->GetExpressionCollection().AddExpression(ColorExpr);

	UMaterialExpressionScalarParameter* OpacityExpr = NewObject<UMaterialExpressionScalarParameter>(Material);
	OpacityExpr->ParameterName = FName("Opacity");
	OpacityExpr->DefaultValue = 1.0f;
	Material->GetExpressionCollection().AddExpression(OpacityExpr);

	UMaterialExpressionScalarParameter* MetallicExpr = NewObject<UMaterialExpressionScalarParameter>(Material);
	MetallicExpr->ParameterName = FName("Metallic");
	MetallicExpr->DefaultValue = 0.0f;
	Material->GetExpressionCollection().AddExpression(MetallicExpr);

	UMaterialExpressionScalarParameter* RoughnessExpr = NewObject<UMaterialExpressionScalarParameter>(Material);
	RoughnessExpr->ParameterName = FName("Roughness");
	RoughnessExpr->DefaultValue = 0.5f;
	Material->GetExpressionCollection().AddExpression(RoughnessExpr);

	UMaterialExpressionScalarParameter* EmissiveExpr = NewObject<UMaterialExpressionScalarParameter>(Material);
	EmissiveExpr->ParameterName = FName("EmissiveStrength");
	EmissiveExpr->DefaultValue = 0.0f;
	Material->GetExpressionCollection().AddExpression(EmissiveExpr);

	UMaterialExpressionVectorParameter* EmissiveColorExpr = NewObject<UMaterialExpressionVectorParameter>(Material);
	EmissiveColorExpr->ParameterName = FName("EmissiveColor");
	EmissiveColorExpr->DefaultValue = FLinearColor::Black;
	Material->GetExpressionCollection().AddExpression(EmissiveColorExpr);

	UMaterialEditorOnlyData* EditorData = Material->GetEditorOnlyData();
	if (EditorData)
	{
		EditorData->BaseColor.Expression = ColorExpr;
		EditorData->BaseColor.OutputIndex = 0;
		EditorData->Opacity.Expression = OpacityExpr;
		EditorData->Opacity.OutputIndex = 0;
		EditorData->Metallic.Expression = MetallicExpr;
		EditorData->Metallic.OutputIndex = 0;
		EditorData->Roughness.Expression = RoughnessExpr;
		EditorData->Roughness.OutputIndex = 0;
		EditorData->EmissiveColor.Expression = EmissiveColorExpr;
		EditorData->EmissiveColor.OutputIndex = 0;
	}

	Material->MarkPackageDirty();
#if WITH_EDITOR
	Material->PostEditChange();
#endif

	CachedBaseMaterial = Material;
	return CachedBaseMaterial;
}

bool UUEMotionAssetFactory::SaveAssetToObject(UObject* Asset, const FString& PackagePath, const FString& AssetName)
{
	if (!Asset) return false;

	UPackage* Package = Asset->GetOutermost();
	if (!Package) return false;

	FString FullAssetPath = ResolveAssetPath(PackagePath, AssetName);

	if (UEditorAssetLibrary::DoesAssetExist(FullAssetPath))
	{
		UEditorAssetLibrary::DeleteAsset(FullAssetPath);
		UE_LOG(LogTemp, Log, TEXT("UEMotionAssetFactory: Deleted existing asset '%s'"), *FullAssetPath);
	}

	Package->MarkPackageDirty();

#if WITH_EDITOR
	Asset->PostEditChange();
#endif

	FAssetRegistryModule::AssetCreated(Asset);

	FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = EObjectFlags::RF_Public | EObjectFlags::RF_Standalone;
	bool bSaved = UPackage::SavePackage(Package, Asset, *Filename, SaveArgs);
	if (bSaved)
	{
		UE_LOG(LogTemp, Log, TEXT("UEMotionAssetFactory: Saved asset '%s' to disk"), *FullAssetPath);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UEMotionAssetFactory: Failed to save asset '%s'"), *FullAssetPath);
	}

	return bSaved;
}

UMaterialInstanceConstant* UUEMotionAssetFactory::CreateAndSaveMaterialAsset(
	const FMotionAssetConfig& Config,
	const FString& AssetName,
	const FString& OutPackagePath)
{
	UMaterialInterface* BaseMaterial = GetOrCreateBaseMaterial();
	if (!BaseMaterial) return nullptr;

	FString FullAssetPath = ResolveAssetPath(OutPackagePath, AssetName);

	if (UEditorAssetLibrary::DoesAssetExist(FullAssetPath))
	{
		UMaterialInstanceConstant* Existing = LoadObject<UMaterialInstanceConstant>(nullptr, *FullAssetPath);
		if (Existing)
		{
			UE_LOG(LogTemp, Log, TEXT("UEMotionAssetFactory: Material '%s' already exists, returning cached"), *FullAssetPath);
			return Existing;
		}
		UEditorAssetLibrary::DeleteAsset(FullAssetPath);
	}

	UPackage* Package = CreatePackage(*FullAssetPath);
	if (!Package) return nullptr;

	FName MaterialName = FName(*AssetName);
	UMaterialInstanceConstant* MIC = NewObject<UMaterialInstanceConstant>(Package, MaterialName, RF_Public | RF_Standalone | RF_Transactional);
	if (!MIC) return nullptr;

	MIC->SetParentEditorOnly(BaseMaterial);

#if WITH_EDITOR
	{
		TArray<FMaterialParameterInfo> ParameterInfos;
		TArray<FGuid> ParameterIds;
		MIC->GetAllScalarParameterInfo(ParameterInfos, ParameterIds);
		MIC->SetScalarParameterValueEditorOnly(FName("Opacity"), Config.Opacity);
		MIC->SetScalarParameterValueEditorOnly(FName("Metallic"), Config.Metallic);
		MIC->SetScalarParameterValueEditorOnly(FName("Roughness"), Config.Roughness);
		MIC->SetScalarParameterValueEditorOnly(FName("EmissiveStrength"), 0.0f);
		MIC->SetVectorParameterValueEditorOnly(FName("BaseColor"), Config.BaseColor);
		MIC->SetVectorParameterValueEditorOnly(FName("EmissiveColor"), FLinearColor::Black);
	}
#endif

	SaveAssetToObject(MIC, OutPackagePath, AssetName);

	UE_LOG(LogTemp, Log, TEXT("UEMotionAssetFactory: Created MaterialInstance '%s' - Color(%s) Metal(%.2f) Rough(%.2f)"),
		*AssetName, *Config.BaseColor.ToString(), Config.Metallic, Config.Roughness);

	return MIC;
}

UClass* UUEMotionAssetFactory::CreateAndSaveBlueprintAsset(
	const FMotionAssetConfig& Config,
	const FString& AssetName,
	const FString& OutPackagePath)
{
	FString FullAssetPath = ResolveAssetPath(OutPackagePath, AssetName);

	if (UEditorAssetLibrary::DoesAssetExist(FullAssetPath))
	{
		UBlueprint* ExistingBP = LoadObject<UBlueprint>(nullptr, *FullAssetPath);
		if (ExistingBP)
		{
			UE_LOG(LogTemp, Log, TEXT("UEMotionAssetFactory: Blueprint '%s' already exists, returning cached"), *FullAssetPath);
			return ExistingBP->GeneratedClass;
		}
		UEditorAssetLibrary::DeleteAsset(FullAssetPath);
	}

	UPackage* Package = CreatePackage(*FullAssetPath);
	if (!Package) return nullptr;

	UBlueprintFactory* BPFactory = NewObject<UBlueprintFactory>();
	BPFactory->ParentClass = AActor::StaticClass();

	UBlueprint* NewBP = Cast<UBlueprint>(BPFactory->FactoryCreateNew(
		UBlueprint::StaticClass(), Package, *AssetName,
		RF_Public | RF_Standalone | RF_Transactional, nullptr, GWarn));
	if (!NewBP)
	{
		UE_LOG(LogTemp, Error, TEXT("UEMotionAssetFactory: Failed to create Blueprint '%s'"), *AssetName);
		return nullptr;
	}

	FAssetRegistryModule::AssetCreated(NewBP);
	NewBP->MarkPackageDirty();

#if WITH_EDITOR
	NewBP->PostEditChange();
#endif

	FString Filename = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = EObjectFlags::RF_Public | EObjectFlags::RF_Standalone;
	UPackage::SavePackage(Package, NewBP, *Filename, SaveArgs);

	UE_LOG(LogTemp, Log, TEXT("UEMotionAssetFactory: Created Blueprint '%s' with mesh '%s'"),
		*AssetName, *Config.GetMeshPath());

	return NewBP->GeneratedClass;
}

AActor* UUEMotionAssetFactory::SpawnFromBlueprintAsset(
	const FString& BlueprintPath,
	const FVector& Location,
	const FRotator& Rotation,
	const FVector& Scale)
{
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("UEMotionAssetFactory::SpawnFromBlueprintAsset: No valid world"));
		return nullptr;
	}

	UBlueprint* BP = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
	if (!BP)
	{
		UClass* LoadedClass = LoadObject<UClass>(nullptr, *BlueprintPath);
		if (!LoadedClass)
		{
			UE_LOG(LogTemp, Error, TEXT("UEMotionAssetFactory: Failed to load Blueprint at '%s'"), *BlueprintPath);
			return nullptr;
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AActor* Actor = World->SpawnActor<AActor>(LoadedClass, Location, Rotation, SpawnParams);
		if (Actor && !Scale.Equals(FVector(1, 1, 1)))
		{
			Actor->SetActorScale3D(Scale);
		}
		return Actor;
	}

	UClass* GeneratedClass = BP->GeneratedClass;
	if (!GeneratedClass)
	{
		UE_LOG(LogTemp, Error, TEXT("UEMotionAssetFactory: Blueprint '%s' has no generated class"), *BlueprintPath);
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AActor* Actor = World->SpawnActor<AActor>(GeneratedClass, Location, Rotation, SpawnParams);
	if (Actor && !Scale.Equals(FVector(1, 1, 1)))
	{
		Actor->SetActorScale3D(Scale);
	}

	UE_LOG(LogTemp, Log, TEXT("UEMotionAssetFactory: Spawned actor from '%s' at (%s)"), *BlueprintPath, *Location.ToString());
	return Actor;
}

bool UUEMotionAssetFactory::DoesAssetExist(const FString& AssetPath)
{
	return UEditorAssetLibrary::DoesAssetExist(AssetPath);
}

UMaterialInstanceDynamic* UUEMotionAssetFactory::CreateDynamicMaterialFromConfig(const FMotionAssetConfig& Config)
{
	UMaterialInterface* BaseMaterial = GetOrCreateBaseMaterial();
	if (!BaseMaterial) return nullptr;

	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMaterial, this);
	if (!MID) return nullptr;

	MID->SetVectorParameterValue(FName("BaseColor"), Config.BaseColor);
	MID->SetScalarParameterValue(FName("Opacity"), Config.Opacity);
	MID->SetScalarParameterValue(FName("Metallic"), Config.Metallic);
	MID->SetScalarParameterValue(FName("Roughness"), Config.Roughness);
	MID->SetScalarParameterValue(FName("EmissiveStrength"), 0.0f);
	MID->SetVectorParameterValue(FName("EmissiveColor"), FLinearColor::Black);

	return MID;
}
