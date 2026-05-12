#include "UEMotionAxisActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"

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
		DynamicMaterial->SetVectorParameterValue(FName("AxisColor"), AxisColor);
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

	static UMaterial* AxisBaseMaterial = nullptr;
	if (!AxisBaseMaterial)
	{
		AxisBaseMaterial = NewObject<UMaterial>(GetTransientPackage(), TEXT("M_AxisBase"), RF_Transient | RF_Public);
		AxisBaseMaterial->MaterialDomain = EMaterialDomain::MD_Surface;
		AxisBaseMaterial->BlendMode = EBlendMode::BLEND_Opaque;

		UMaterialExpressionVectorParameter* ColorParam = NewObject<UMaterialExpressionVectorParameter>(AxisBaseMaterial);
		ColorParam->ParameterName = FName("AxisColor");
		ColorParam->DefaultValue = FLinearColor::White;
		AxisBaseMaterial->GetExpressionCollection().AddExpression(ColorParam);

		UMaterialEditorOnlyData* EditorData = AxisBaseMaterial->GetEditorOnlyData();
		if (EditorData)
		{
			EditorData->BaseColor.Expression = ColorParam;
			EditorData->BaseColor.OutputIndex = 0;
		}

		AxisBaseMaterial->MarkPackageDirty();
#if WITH_EDITOR
		AxisBaseMaterial->PostEditChange();
#endif
	}

	DynamicMaterial = UMaterialInstanceDynamic::Create(AxisBaseMaterial, this);
	if (DynamicMaterial)
	{
		DynamicMaterial->SetVectorParameterValue(FName("AxisColor"), AxisColor);
		MeshComponent->SetMaterial(0, DynamicMaterial);
	}
}
