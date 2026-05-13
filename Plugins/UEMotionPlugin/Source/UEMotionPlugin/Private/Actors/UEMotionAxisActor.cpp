#include "UEMotionAxisActor.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInstanceConstant.h"
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

	LineMeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("LineMeshComponent"));
	SetRootComponent(LineMeshComponent);

	LineMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LineMeshComponent->SetMobility(EComponentMobility::Movable);
	LineMeshComponent->bReceivesDecals = false;
	LineMeshComponent->CastShadow = false;
	LineMeshComponent->bUseComplexAsSimpleCollision = false;

	AxisType = EAxis::X;
	AxisLength = 400.0f;
	AxisColor = FLinearColor::Red;
	LineThickness = 2.0f;
	LineSegments = 8;
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

	if (LineMeshComponent)
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
		LineMeshComponent->SetRelativeRotation(TargetRotation);
	}
}

void AUEMotionAxisActor::SetAxisColor(const FLinearColor& InColor)
{
	AxisColor = InColor;

	if (LineMeshComponent && AxisMaterial)
	{
		LineMeshComponent->SetMaterial(0, AxisMaterial);
	}
	else
	{
		CreateOrLoadAxisMaterial();
	}
}

UProceduralMeshComponent* AUEMotionAxisActor::GetMeshComponent() const
{
	return LineMeshComponent;
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

FVector AUEMotionAxisActor::GetAxisDirection() const
{
	switch (static_cast<EAxis::Type>(AxisType.GetValue()))
	{
	case EAxis::X:
		return FVector::ForwardVector;
	case EAxis::Y:
		return FVector::RightVector;
	case EAxis::Z:
		return FVector::UpVector;
	default:
		return FVector::ForwardVector;
	}
}

void AUEMotionAxisActor::SetupMesh()
{
	if (!LineMeshComponent) return;

	LineMeshComponent->ClearAllMeshSections();

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> Colors;
	TArray<FProcMeshTangent> Tangents;

	const float HalfLength = AxisLength * 0.5f;
	const float Radius = FMath::Max(LineThickness * 0.5f, 0.05f);
	const int32 Segments = FMath::Max(LineSegments, 3);

	FVector Direction = GetAxisDirection();
	FVector Perp1, Perp2;
	Direction.FindBestAxisVectors(Perp1, Perp2);

	FProcMeshTangent BaseTangent(FVector::CrossProduct(Direction, Perp1).GetSafeNormal(), false);

	for (int32 i = 0; i <= Segments; ++i)
	{
		float Angle = (float)i / Segments * 2.0f * PI;
		float CosA = FMath::Cos(Angle);
		float SinA = FMath::Sin(Angle);

		FVector Offset = (Perp1 * CosA + Perp2 * SinA) * Radius;
		FVector Normal = (Perp1 * CosA + Perp2 * SinA).GetSafeNormal();

		int32 BottomIdx = Vertices.Add(-Direction * HalfLength + Offset);
		int32 TopIdx = Vertices.Add(Direction * HalfLength + Offset);

		Normals.Add(Normal);
		Normals.Add(Normal);

		UVs.Add(FVector2D((float)i / Segments, 0.0f));
		UVs.Add(FVector2D((float)i / Segments, 1.0f));

		Colors.Add(AxisColor);
		Colors.Add(AxisColor);

		Tangents.Add(BaseTangent);
		Tangents.Add(BaseTangent);

		if (i > 0)
		{
			int32 PrevBottom = BottomIdx - 2;
			int32 PrevTop = TopIdx - 2;

			Triangles.Add(PrevBottom); Triangles.Add(BottomIdx); Triangles.Add(TopIdx);
			Triangles.Add(PrevBottom); Triangles.Add(TopIdx); Triangles.Add(PrevTop);
		}
	}

	int32 LastBottom = Segments * 2;
	int32 LastTop = Segments * 2 + 1;
	int32 FirstBottom = 0;
	int32 FirstTop = 1;

	Triangles.Add(LastBottom); Triangles.Add(FirstBottom); Triangles.Add(FirstTop);
	Triangles.Add(LastBottom); Triangles.Add(FirstTop); Triangles.Add(LastTop);

	LineMeshComponent->CreateMeshSection_LinearColor(
		0, Vertices, Triangles, Normals, UVs, Colors, Tangents, true);

	CreateOrLoadAxisMaterial();

	LineMeshComponent->SetVisibility(true);

	UE_LOG(LogTemp, Log,
		TEXT("UEMotionAxisActor: Created axis line mesh [Axis=%d, Length=%.1f, Thickness=%.1f, Segments=%d]"),
		static_cast<int32>(AxisType.GetValue()), AxisLength, LineThickness, Segments);
}

void AUEMotionAxisActor::ApplyRotationForAxis()
{
	if (!LineMeshComponent) return;

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

	LineMeshComponent->SetRelativeRotation(TargetRotation);
}

void AUEMotionAxisActor::CreateOrLoadAxisMaterial()
{
	if (!LineMeshComponent) return;

	if (LineMeshComponent->GetMaterial(0))
	{
		AxisMaterial = LineMeshComponent->GetMaterial(0);
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
		LineMeshComponent->SetMaterial(0, AxisMaterial);
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
