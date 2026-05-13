#include "UEMotionAxisActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "EditorAssetLibrary.h"
#include "PackageTools.h"
#include "FileHelpers.h"
#include "Utils/UEMotionSequencerCompat.h"

AUEMotionAxisActor::AUEMotionAxisActor()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);

	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetMobility(EComponentMobility::Movable);
	MeshComponent->bReceivesDecals = false;
	MeshComponent->CastShadow = false;

	AxisType = EAxis::X;
	AxisLength = 200.0f;
	AxisColor = FLinearColor::Red;

	MeshComponent->SetRelativeScale3D(FVector(5.0f, 0.2f, 0.2f));
}

void AUEMotionAxisActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	SetupMesh();
}

void AUEMotionAxisActor::InitializeAxis(EAxis::Type InAxis, float InLength, const FLinearColor& InColor)
{
	AxisType = InAxis;
	AxisLength = FMath::Max(InLength, 10.0f);
	AxisColor = InColor;

	SetupMesh();

	if (MeshComponent)
	{
		FRotator TargetRotation = FRotator::ZeroRotator;
		switch (InAxis)
		{
		case EAxis::X:
			TargetRotation = FRotator(0, 0, 0);
			break;
		case EAxis::Y:
			TargetRotation = FRotator(0, -90, 0);
			break;
		case EAxis::Z:
			TargetRotation = FRotator(90, 0, 0);
			break;
		default:
			TargetRotation = FRotator::ZeroRotator;
			break;
		}
		MeshComponent->SetRelativeRotation(TargetRotation);
	}
}

void AUEMotionAxisActor::SetAxisColor(const FLinearColor& InColor)
{
	AxisColor = InColor;

	if (MeshComponent && AxisMaterial)
	{
		MeshComponent->SetMaterial(0, AxisMaterial);
	}
	else
	{
		CreateOrLoadAxisMaterial();
	}
}

UStaticMeshComponent* AUEMotionAxisActor::GetMeshComponent() const
{
	return MeshComponent;
}

bool AUEMotionAxisActor::CreateAxisMaterials()
{
	bool bSuccess = true;

	bSuccess &= CreateStaticAxisMaterial(TEXT("M_Axis_X"), FLinearColor(1.0f, 0.25f, 0.25f, 1.0f)) != nullptr;
	bSuccess &= CreateStaticAxisMaterial(TEXT("M_Axis_Y"), FLinearColor(0.25f, 1.0f, 0.25f, 1.0f)) != nullptr;
	bSuccess &= CreateStaticAxisMaterial(TEXT("M_Axis_Z"), FLinearColor(0.25f, 0.5f, 1.0f, 1.0f)) != nullptr;

	if (bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("UEMotionAxisActor: All axis materials created successfully"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("UEMotionAxisActor: Some axis materials failed to create"));
	}

	return bSuccess;
}

void AUEMotionAxisActor::SetupMesh()
{
	if (!MeshComponent) return;

	static const FString GizmoMeshPath = TEXT("/Engine/InteractiveToolsFramework/Meshes/GizmoArrowHandle.GizmoArrowHandle");
	UStaticMesh* GizmoMesh = LoadObject<UStaticMesh>(nullptr, *GizmoMeshPath);

	if (GizmoMesh)
	{
		MeshComponent->SetStaticMesh(GizmoMesh);
	}

	CreateOrLoadAxisMaterial();

	MeshComponent->SetVisibility(true);
}

void AUEMotionAxisActor::ApplyRotationForAxis()
{
	if (!MeshComponent) return;

	FRotator TargetRotation = FRotator::ZeroRotator;

	switch (static_cast<EAxis::Type>(AxisType.GetValue()))
	{
	case EAxis::X:
		TargetRotation = FRotator(0, 0, 0);
		break;
	case EAxis::Y:
		TargetRotation = FRotator(0, -90, 0);
		break;
	case EAxis::Z:
		TargetRotation = FRotator(90, 0, 0);
		break;
	default:
		TargetRotation = FRotator::ZeroRotator;
		break;
	}

	MeshComponent->SetRelativeRotation(TargetRotation);
}

void AUEMotionAxisActor::CreateOrLoadAxisMaterial()
{
	if (!MeshComponent) return;

	if (MeshComponent->GetMaterial(0))
	{
		AxisMaterial = MeshComponent->GetMaterial(0);
		UE_LOG(LogTemp, Log, TEXT("UEMotionAxisActor: Using material assigned in blueprint"));
		return;
	}

	FString MaterialName;
	switch (static_cast<EAxis::Type>(AxisType.GetValue()))
	{
	case EAxis::X:
		MaterialName = TEXT("M_Axis_X");
		break;
	case EAxis::Y:
		MaterialName = TEXT("M_Axis_Y");
		break;
	case EAxis::Z:
		MaterialName = TEXT("M_Axis_Z");
		break;
	default:
		MaterialName = TEXT("M_Axis_X");
		break;
	}

	FString MaterialPath = FString::Printf(TEXT("/Game/UEMotion/Materials/%s"), *MaterialName);

	AxisMaterial = LoadObject<UMaterialInterface>(nullptr, *MaterialPath);
	if (!AxisMaterial)
	{
		AxisMaterial = CreateStaticAxisMaterial(MaterialName, AxisColor);
	}

	if (AxisMaterial)
	{
		MeshComponent->SetMaterial(0, AxisMaterial);
	}
}

UMaterialInterface* AUEMotionAxisActor::CreateStaticAxisMaterial(const FString& MaterialName, const FLinearColor& Color)
{
	FString MaterialPath = FString::Printf(TEXT("/Game/UEMotion/Materials/%s"), *MaterialName);

	if (UEditorAssetLibrary::DoesAssetExist(MaterialPath))
	{
		UMaterialInterface* ExistingMat = LoadObject<UMaterialInterface>(nullptr, *MaterialPath);
		if (ExistingMat)
		{
			UE_LOG(LogTemp, Log, TEXT("UEMotionAxisActor: Material '%s' already exists, loading"), *MaterialName);
			return ExistingMat;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("UEMotionAxisActor: Material '%s' exists but failed to load, recreating..."), *MaterialName);
			UEditorAssetLibrary::DeleteAsset(MaterialPath);
		}
	}

	static UMaterialInterface* BaseMaterial = nullptr;
	if (!BaseMaterial)
	{
		BaseMaterial = LoadObject<UMaterialInterface>(
			nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	}

	if (!BaseMaterial)
	{
		UE_LOG(LogTemp, Error, TEXT("UEMotionAxisActor: Failed to load base material"));
		return nullptr;
	}

	UPackage* MaterialPackage = CreatePackage(*MaterialPath);
	if (!MaterialPackage)
	{
		UE_LOG(LogTemp, Error, TEXT("UEMotionAxisActor: Failed to create package for material '%s'"), *MaterialName);
		return nullptr;
	}

	UMaterialInstanceConstant* NewMaterialInstance = NewObject<UMaterialInstanceConstant>(
		MaterialPackage, *MaterialName, RF_Public | RF_Standalone | RF_Transactional);

	if (!NewMaterialInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("UEMotionAxisActor: Failed to create material instance '%s'"), *MaterialName);
		return nullptr;
	}

	NewMaterialInstance->SetParentEditorOnly(BaseMaterial);

#if WITH_EDITOR
	NewMaterialInstance->SetVectorParameterValueEditorOnly(FName("Color"), Color);
	NewMaterialInstance->PostEditChange();
#endif

	NewMaterialInstance->SetFlags(RF_Public | RF_Standalone);
	FAssetRegistryModule::AssetCreated(NewMaterialInstance);
	MaterialPackage->MarkPackageDirty();

	FString FilePath = FPaths::Combine(
		FPaths::ProjectContentDir(),
		TEXT("UEMotion/Materials/"),
		MaterialName + TEXT(".uasset"));

	FSavePackageArgs SaveArgs;
	SaveArgs.SaveFlags = RF_Public | RF_Standalone;

	bool bSaveSuccess = UPackage::SavePackage(
		MaterialPackage,
		NewMaterialInstance,
		*FilePath,
		SaveArgs);

	if (bSaveSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("UEMotionAxisActor: Created and saved axis material instance '%s' to '%s' with color (%.2f, %.2f, %.2f)"),
			*MaterialName, *FilePath, Color.R, Color.G, Color.B);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("UEMotionAxisActor: Failed to save axis material '%s' to '%s'"), *MaterialName, *FilePath);
		return nullptr;
	}

	return NewMaterialInstance;
}
