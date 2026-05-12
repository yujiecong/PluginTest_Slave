#include "UEMotionMobject.h"
#include "Actors/UEMotionSceneActor.h"
#include "Actors/UEMotionMobjectActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionConstant.h"
#include "UObject/ConstructorHelpers.h"

UMaterialInterface* UUEMotionMobject::GetOrCreateBaseMaterial()
{
	if (CachedBaseMaterial) return CachedBaseMaterial;

	UMaterial* Material = NewObject<UMaterial>(GetTransientPackage(), TEXT("M_UEMotionBase"), RF_Transient | RF_Public);
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

	UMaterialExpressionScalarParameter* OpacityParam = NewObject<UMaterialExpressionScalarParameter>(Material);
	OpacityParam->ParameterName = FName("Opacity");
	OpacityParam->DefaultValue = 1.0f;
	Material->GetExpressionCollection().AddExpression(OpacityParam);
	UMaterialExpression* OpacityOutput = OpacityParam;

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
		EditorData->EmissiveColor.Expression = ColorExpr;
		EditorData->EmissiveColor.OutputIndex = 0;
		EditorData->Opacity.Expression = OpacityOutput;
		EditorData->Opacity.OutputIndex = 0;
		EditorData->Roughness.Expression = RoughnessConst;
		EditorData->Roughness.OutputIndex = 0;
		EditorData->Metallic.Expression = MetallicConst;
		EditorData->Metallic.OutputIndex = 0;
	}

	Material->MarkPackageDirty();
#if WITH_EDITOR
	Material->PostEditChange();
#endif

	CachedBaseMaterial = Material;
	return CachedBaseMaterial;
}

void UUEMotionMobject::InitializeAsSphere(AUEMotionSceneActor* Owner, float Radius)
{
	CreateStaticMeshActor(Owner, TEXT("/Engine/BasicShapes/Sphere.Sphere"), Radius / 50.0f);
	MobjectName = TEXT("Sphere");
}

void UUEMotionMobject::InitializeAsMesh(AUEMotionSceneActor* Owner, const FString& MeshPath, float Scale)
{
	CreateStaticMeshActor(Owner, MeshPath, Scale / 50.0f);
}

void UUEMotionMobject::CreateStaticMeshActor(AUEMotionSceneActor* Owner, const FString& MeshPath, float InScale)
{
	if (!Owner) return;

	UWorld* World = Owner->GetWorld();
	if (!World) return;

	FActorSpawnParameters Params;
	Params.Name = FName(*FString::Printf(TEXT("Mobject_%d"), GetUniqueID()));
	AUEMotionMobjectActor* Actor = World->SpawnActor<AUEMotionMobjectActor>(
		AUEMotionMobjectActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
	if (!Actor) return;

	InternalActor = Actor;

	UStaticMeshComponent* MeshComp = NewObject<UStaticMeshComponent>(Actor, TEXT("Mesh"));
	MeshComp->RegisterComponent();
	Actor->SetRootComponent(MeshComp);

	UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *MeshPath);
	if (Mesh)
	{
		MeshComp->SetStaticMesh(Mesh);
	}

	UMaterialInterface* BaseMaterial = GetOrCreateBaseMaterial();
	if (BaseMaterial && MeshComp)
	{
		MaterialInstance = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		MeshComp->SetMaterial(0, MaterialInstance.Get());

		if (MaterialInstance.IsValid())
		{
			MaterialInstance->SetVectorParameterValue(FName("BaseColor"), CurrentColor);
			MaterialInstance->SetScalarParameterValue(FName("Opacity"), CurrentOpacity);
		}
	}

	MeshComponent = MeshComp;

	Actor->EnsureDynamicMaterial();

	if (!FMath::IsNearlyEqual(InScale, 1.0f))
	{
		Actor->SetActorScale3D(FVector(InScale));
	}
}

void UUEMotionMobject::SetLocation(const FVector& Location)
{
	if (InternalActor.IsValid())
	{
		InternalActor->SetActorLocation(Location);
	}
}

FVector UUEMotionMobject::GetLocation() const
{
	if (InternalActor.IsValid())
		return InternalActor->GetActorLocation();
	return FVector::ZeroVector;
}

void UUEMotionMobject::SetColor(const FLinearColor& Color)
{
	CurrentColor = Color;
	if (MaterialInstance.IsValid())
	{
		MaterialInstance->SetVectorParameterValue(FName("BaseColor"), Color);
	}
}

FLinearColor UUEMotionMobject::GetColor() const
{
	return CurrentColor;
}

void UUEMotionMobject::SetVisibility(bool bInVisible)
{
	bVisible = bInVisible;
	if (MeshComponent.IsValid())
	{
		MeshComponent->SetVisibility(bVisible);
	}
}

bool UUEMotionMobject::GetVisibility() const
{
	return bVisible;
}

void UUEMotionMobject::SetScale(const FVector& Scale)
{
	if (InternalActor.IsValid())
	{
		InternalActor->SetActorScale3D(Scale);
	}
}

FVector UUEMotionMobject::GetScale() const
{
	if (InternalActor.IsValid())
		return InternalActor->GetActorScale3D();
	return FVector::OneVector;
}

void UUEMotionMobject::SetRotation(const FRotator& Rotation)
{
	if (InternalActor.IsValid())
	{
		InternalActor->SetActorRotation(Rotation);
	}
}

FRotator UUEMotionMobject::GetRotation() const
{
	if (InternalActor.IsValid())
		return InternalActor->GetActorRotation();
	return FRotator::ZeroRotator;
}

void UUEMotionMobject::SetOpacity(float InOpacity)
{
	CurrentOpacity = FMath::Clamp(InOpacity, 0.0f, 1.0f);

	if (MaterialInstance.IsValid())
	{
		MaterialInstance->SetScalarParameterValue(FName("Opacity"), CurrentOpacity);
	}

	if (CurrentOpacity <= 0.0f)
	{
		bVisible = false;
	}
	else
	{
		bVisible = true;
	}
}

float UUEMotionMobject::GetOpacity() const
{
	return CurrentOpacity;
}

void UUEMotionMobject::Destroy()
{
	if (InternalActor.IsValid())
	{
		InternalActor->Destroy();
		InternalActor = nullptr;
	}
	MeshComponent = nullptr;
	MaterialInstance = nullptr;
}

void UUEMotionMobject::InitializeFromSpawnedActor(AActor* SpawnedActor, UStaticMeshComponent* MeshComp)
{
	if (!SpawnedActor) return;

	InternalActor = SpawnedActor;
	MeshComponent = MeshComp;

	if (MeshComp)
	{
		UMaterialInterface* CurrentMat = MeshComp->GetMaterial(0);
		if (CurrentMat)
		{
			MaterialInstance = Cast<UMaterialInstanceDynamic>(CurrentMat);
			if (MaterialInstance.IsValid())
			{
				FLinearColor OutColor;
				if (MaterialInstance->GetVectorParameterValue(FName("BaseColor"), OutColor))
				{
					CurrentColor = OutColor;
				}
				float OutOpacity;
				if (MaterialInstance->GetScalarParameterValue(FName("Opacity"), OutOpacity))
				{
					CurrentOpacity = OutOpacity;
				}
			}
		}
	}

	AUEMotionMobjectActor* MobActor = Cast<AUEMotionMobjectActor>(SpawnedActor);
	if (MobActor)
	{
		MobActor->EnsureDynamicMaterial();
		if (MobActor->GetDynamicMaterial())
		{
			MaterialInstance = MobActor->GetDynamicMaterial();
		}
	}

	if (MaterialInstance.IsValid())
	{
		MaterialInstance->SetVectorParameterValue(FName("BaseColor"), CurrentColor);
		MaterialInstance->SetScalarParameterValue(FName("Opacity"), CurrentOpacity);
	}

	UE_LOG(LogTemp, Log, TEXT("UEMotionMobject: Initialized from spawned actor '%s' (AssetBased=%s)"),
		*SpawnedActor->GetName(), bIsAssetBased ? TEXT("true") : TEXT("false"));
}

FString UUEMotionMobject::GetName() const
{
	return MobjectName;
}

void UUEMotionMobject::GetBounds(FVector& OutOrigin, FVector& OutBoxExtent) const
{
	if (InternalActor.IsValid())
	{
		InternalActor->GetActorBounds(false, OutOrigin, OutBoxExtent);
	}
	else
	{
		OutOrigin = FVector::ZeroVector;
		OutBoxExtent = FVector::ZeroVector;
	}
}
