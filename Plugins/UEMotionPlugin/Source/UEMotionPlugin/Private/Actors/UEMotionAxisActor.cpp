#include "UEMotionAxisActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

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
}

void AUEMotionAxisActor::SetAxisColor(const FLinearColor& InColor)
{
	AxisColor = InColor;

	if (MeshComponent && DynamicMaterial)
	{
		DynamicMaterial->SetVectorParameterValue(FName("BaseColor"), AxisColor);
	}
	else
	{
		CreateAxisMaterial();
	}
}

UStaticMeshComponent* AUEMotionAxisActor::GetMeshComponent() const
{
	return MeshComponent;
}

void AUEMotionAxisActor::SetupMesh()
{
	if (!MeshComponent) return;

	static const FString GizmoMeshPath = TEXT("/Engine/InteractiveToolsFramework/Meshes/GizmoArrowHandle.GizmoArrowHandle");
	UStaticMesh* GizmoMesh = LoadObject<UStaticMesh>(nullptr, *GizmoMeshPath);

	if (GizmoMesh)
	{
		MeshComponent->SetStaticMesh(GizmoMesh);

		float Len = AxisLength;
		float Thickness = 0.6f;
		MeshComponent->SetWorldScale3D(FVector(Len / 60.0f, Thickness / 60.0f, Thickness / 60.0f));
	}

	ApplyRotationForAxis();
	CreateAxisMaterial();

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
		TargetRotation = FRotator(0, 90, 0);
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

void AUEMotionAxisActor::CreateAxisMaterial()
{
	if (!MeshComponent) return;

	static UMaterialInterface* AxisBaseMaterial = nullptr;
	if (!AxisBaseMaterial)
	{
		AxisBaseMaterial = LoadObject<UMaterialInterface>(
			nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	}

	if (!AxisBaseMaterial) return;

	DynamicMaterial = UMaterialInstanceDynamic::Create(AxisBaseMaterial, this);
	if (DynamicMaterial)
	{
		DynamicMaterial->SetVectorParameterValue(FName("BaseColor"), AxisColor);
		DynamicMaterial->SetScalarParameterValue(FName("Opacity"), 1.0f);
		MeshComponent->SetMaterial(0, DynamicMaterial);
	}
}
