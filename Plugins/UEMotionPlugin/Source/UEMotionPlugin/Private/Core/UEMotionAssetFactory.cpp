#include "UEMotionAssetFactory.h"
#include "Actors/UEMotionMobjectActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionCollectionParameter.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialParameterCollection.h"
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
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "EditorFramework/AssetImportData.h"
#include "PackageHelperFunctions.h"

const FString UUEMotionAssetFactory::TranslucentMaterialPath = TEXT("/Game/UEMotion/Materials/M_UEMotionBaseTranslucent");
static const FString FadeMPCPath = TEXT("/Game/UEMotion/Materials/MPC_UEMotionFade");

FString UUEMotionAssetFactory::ResolveAssetPath(const FString& PackagePath, const FString& AssetName) const
{
	return FString::Printf(TEXT("%s/%s"), *PackagePath, *AssetName);
}

UMaterialInterface* UUEMotionAssetFactory::EnsureBaseTranslucentMaterial()
{
	if (UEditorAssetLibrary::DoesAssetExist(TranslucentMaterialPath))
	{
		UMaterialInterface* Existing = LoadObject<UMaterialInterface>(nullptr, *TranslucentMaterialPath);
		if (Existing) return Existing;
	}

	UMaterialParameterCollection* MPC = EnsureFadeMPC();

	FString PackageName = TranslucentMaterialPath;
	UPackage* Package = CreatePackage(*PackageName);
	if (!Package) return nullptr;

	UMaterial* Material = NewObject<UMaterial>(Package, FName(TEXT("M_UEMotionBaseTranslucent")), RF_Public | RF_Standalone);
	if (!Material) return nullptr;

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

	UMaterialExpressionCollectionParameter* MPCNode = NewObject<UMaterialExpressionCollectionParameter>(Material);
	MPCNode->Collection = MPC;
	MPCNode->ParameterName = FName("Opacity");
	Material->GetExpressionCollection().AddExpression(MPCNode);

	UMaterialExpressionMultiply* MultiplyNode = NewObject<UMaterialExpressionMultiply>(Material);
	MultiplyNode->A.Expression = OpacityExpr;
	MultiplyNode->A.OutputIndex = 0;
	MultiplyNode->B.Expression = MPCNode;
	MultiplyNode->B.OutputIndex = 0;
	Material->GetExpressionCollection().AddExpression(MultiplyNode);

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
		EditorData->Opacity.Expression = MultiplyNode;
		EditorData->Opacity.OutputIndex = 0;
		EditorData->Roughness.Expression = RoughnessConst;
		EditorData->Roughness.OutputIndex = 0;
		EditorData->Metallic.Expression = MetallicConst;
		EditorData->Metallic.OutputIndex = 0;
	}

	SaveAssetToObject(Material, TEXT("/Game/UEMotion/Materials"), TEXT("M_UEMotionBaseTranslucent"));

	UE_LOG(LogTemp, Log, TEXT("UEMotionAssetFactory: Created persistent translucent base material '%s'"), *TranslucentMaterialPath);

	FAssetRegistryModule::AssetCreated(Material);
	Material->MarkPackageDirty();
#if WITH_EDITOR
	Material->PostEditChange();
#endif

	return Material;
}

UMaterialParameterCollection* UUEMotionAssetFactory::EnsureFadeMPC()
{
	if (UEditorAssetLibrary::DoesAssetExist(FadeMPCPath))
	{
		UMaterialParameterCollection* Existing = LoadObject<UMaterialParameterCollection>(nullptr, *FadeMPCPath);
		if (Existing) return Existing;
	}

	UPackage* Package = CreatePackage(*FadeMPCPath);
	if (!Package) return nullptr;

	UMaterialParameterCollection* MPC = NewObject<UMaterialParameterCollection>(
		Package, FName(TEXT("MPC_UEMotionFade")), RF_Public | RF_Standalone);
	if (!MPC) return nullptr;

	FCollectionScalarParameter ScalarParam;
	ScalarParam.ParameterName = FName("Opacity");
	ScalarParam.DefaultValue = 1.0f;
	MPC->ScalarParameters.Add(ScalarParam);

	SaveAssetToObject(MPC, TEXT("/Game/UEMotion/Materials"), TEXT("MPC_UEMotionFade"));

	UE_LOG(LogTemp, Log, TEXT("UEMotionAssetFactory: Created MPC '%s'"), *FadeMPCPath);

	FAssetRegistryModule::AssetCreated(MPC);
	MPC->MarkPackageDirty();

	return MPC;
}

UMaterialInterface* UUEMotionAssetFactory::GetOrCreateBaseMaterial()
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

bool UUEMotionAssetFactory::SaveAssetToObject(UObject* Asset, const FString& PackagePath, const FString& AssetName)
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
		FString FullAssetPath = ResolveAssetPath(PackagePath, AssetName);
		UE_LOG(LogTemp, Log, TEXT("UEMotionAssetFactory: Saved asset '%s'"), *FullAssetPath);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UEMotionAssetFactory: Failed to save asset '%s'"), *AssetName);
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
			return Existing;
		}
	}

	UPackage* Package = CreatePackage(*FullAssetPath);
	if (!Package) return nullptr;

	FName MaterialName = FName(*AssetName);
	UMaterialInstanceConstant* MIC = NewObject<UMaterialInstanceConstant>(Package, MaterialName, RF_Public | RF_Standalone);
	if (!MIC) return nullptr;

	MIC->SetParentEditorOnly(BaseMaterial);

#if WITH_EDITOR
	MIC->SetScalarParameterValueEditorOnly(FName("Opacity"), Config.Opacity);
	MIC->SetScalarParameterValueEditorOnly(FName("Metallic"), Config.Metallic);
	MIC->SetScalarParameterValueEditorOnly(FName("Roughness"), Config.Roughness);
	MIC->SetScalarParameterValueEditorOnly(FName("EmissiveStrength"), 0.0f);
	MIC->SetVectorParameterValueEditorOnly(FName("BaseColor"), Config.BaseColor);
	MIC->SetVectorParameterValueEditorOnly(FName("EmissiveColor"), FLinearColor::Black);
#endif

	SaveAssetToObject(MIC, OutPackagePath, AssetName);

	UE_LOG(LogTemp, Log, TEXT("UEMotionAssetFactory: Created MaterialInstance '%s'"), *AssetName);
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
		if (ExistingBP && ExistingBP->GeneratedClass)
		{
			UE_LOG(LogTemp, Log, TEXT("UEMotionAssetFactory: Blueprint '%s' already exists, reusing"), *FullAssetPath);
			return ExistingBP->GeneratedClass;
		}
	}

	FString MaterialAssetName = FString::Printf(TEXT("MI_%s"), *AssetName);
	FString MaterialPackagePath = OutPackagePath.Replace(TEXT("Blueprints"), TEXT("Materials"));
	UMaterialInstanceConstant* MIC = CreateAndSaveMaterialAsset(Config, MaterialAssetName, MaterialPackagePath);

	UPackage* Package = CreatePackage(*FullAssetPath);
	if (!Package) return nullptr;

	UBlueprintFactory* BPFactory = NewObject<UBlueprintFactory>();
	BPFactory->ParentClass = AUEMotionMobjectActor::StaticClass();

	UBlueprint* NewBP = Cast<UBlueprint>(BPFactory->FactoryCreateNew(
		UBlueprint::StaticClass(), Package, *AssetName,
		RF_Public | RF_Standalone, nullptr, GWarn));
	if (!NewBP)
	{
		UE_LOG(LogTemp, Error, TEXT("UEMotionAssetFactory: Failed to create Blueprint '%s'"), *AssetName);
		return nullptr;
	}

	USimpleConstructionScript* SCS = NewBP->SimpleConstructionScript;
	if (SCS)
	{
		USCS_Node* RootNode = SCS->CreateNode(USceneComponent::StaticClass(), TEXT("DefaultSceneRoot"));
		SCS->AddNode(RootNode);

		USCS_Node* MeshNode = SCS->CreateNode(UStaticMeshComponent::StaticClass(), TEXT("Mesh"));
		RootNode->AddChildNode(MeshNode);

		UStaticMeshComponent* MeshTemplate = Cast<UStaticMeshComponent>(MeshNode->ComponentTemplate);
		if (MeshTemplate)
		{
			UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Config.GetMeshPath());
			if (Mesh)
			{
				MeshTemplate->SetStaticMesh(Mesh);
			}

			if (MIC)
			{
				MeshTemplate->SetMaterial(0, MIC);
			}

			float ScaleFactor = Config.Size / 50.0f;
			if (!FMath::IsNearlyEqual(ScaleFactor, 1.0f))
			{
				MeshTemplate->SetRelativeScale3D(FVector(ScaleFactor));
			}
		}
	}

	FKismetEditorUtilities::CompileBlueprint(NewBP);

	SaveAssetToObject(NewBP, OutPackagePath, AssetName);

	UE_LOG(LogTemp, Log, TEXT("UEMotionAssetFactory: Created Blueprint '%s'"), *AssetName);
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

	UClass* SpawnClass = nullptr;

	UBlueprint* BP = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
	if (BP && BP->GeneratedClass)
	{
		SpawnClass = BP->GeneratedClass;
	}
	else
	{
		SpawnClass = LoadObject<UClass>(nullptr, *BlueprintPath);
	}

	if (!SpawnClass)
	{
		UE_LOG(LogTemp, Error, TEXT("UEMotionAssetFactory: Failed to load class from '%s'"), *BlueprintPath);
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AActor* Actor = World->SpawnActor<AActor>(SpawnClass, Location, Rotation, SpawnParams);
	if (Actor && !Scale.Equals(FVector(1, 1, 1)))
	{
		Actor->SetActorScale3D(Scale);
	}

	UE_LOG(LogTemp, Log, TEXT("UEMotionAssetFactory: Spawned actor from '%s'"), *BlueprintPath);
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
