#include "PyManimMobject.h"
#include "Actors/PyManimSceneActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

void UPyManimMobject::InitializeAsSphere(APyManimSceneActor* Owner, float Radius)
{
	CreateStaticMeshActor(Owner, TEXT("/Engine/BasicShapes/Sphere.Sphere"), Radius / 50.0f);
	MobjectName = TEXT("Sphere");
}

void UPyManimMobject::InitializeAsMesh(APyManimSceneActor* Owner, const FString& MeshPath, float Scale)
{
	CreateStaticMeshActor(Owner, MeshPath, Scale / 50.0f);
}

void UPyManimMobject::CreateStaticMeshActor(APyManimSceneActor* Owner, const FString& MeshPath, float InScale)
{
	if (!Owner) return;

	UWorld* World = Owner->GetWorld();
	if (!World) return;

	FActorSpawnParameters Params;
	Params.Name = FName(*FString::Printf(TEXT("Mobject_%d"), GetUniqueID()));
	AActor* Actor = World->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
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

	UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(
		nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	if (BaseMaterial && MeshComp)
	{
		MaterialInstance = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		MeshComp->SetMaterial(0, MaterialInstance.Get());
	}

	MeshComponent = MeshComp;

	if (!FMath::IsNearlyEqual(InScale, 1.0f))
	{
		Actor->SetActorScale3D(FVector(InScale));
	}
}

void UPyManimMobject::SetLocation(const FVector& Location)
{
	if (InternalActor.IsValid())
	{
		InternalActor->SetActorLocation(Location);
	}
}

FVector UPyManimMobject::GetLocation() const
{
	if (InternalActor.IsValid())
		return InternalActor->GetActorLocation();
	return FVector::ZeroVector;
}

void UPyManimMobject::SetColor(const FLinearColor& Color)
{
	CurrentColor = Color;
	if (MaterialInstance.IsValid())
	{
		MaterialInstance->SetVectorParameterValue(TEXT("BaseColor"), Color);
	}
}

FLinearColor UPyManimMobject::GetColor() const
{
	return CurrentColor;
}

void UPyManimMobject::SetVisibility(bool bInVisible)
{
	bVisible = bInVisible;
	if (MeshComponent.IsValid())
	{
		MeshComponent->SetVisibility(bVisible);
	}
}

bool UPyManimMobject::GetVisibility() const
{
	return bVisible;
}

void UPyManimMobject::SetScale(const FVector& Scale)
{
	if (InternalActor.IsValid())
	{
		InternalActor->SetActorScale3D(Scale);
	}
}

FVector UPyManimMobject::GetScale() const
{
	if (InternalActor.IsValid())
		return InternalActor->GetActorScale3D();
	return FVector::OneVector;
}

void UPyManimMobject::SetRotation(const FRotator& Rotation)
{
	if (InternalActor.IsValid())
	{
		InternalActor->SetActorRotation(Rotation);
	}
}

FRotator UPyManimMobject::GetRotation() const
{
	if (InternalActor.IsValid())
		return InternalActor->GetActorRotation();
	return FRotator::ZeroRotator;
}

void UPyManimMobject::Destroy()
{
	if (InternalActor.IsValid())
	{
		InternalActor->Destroy();
		InternalActor = nullptr;
	}
	MeshComponent = nullptr;
	MaterialInstance = nullptr;
}

FString UPyManimMobject::GetName() const
{
	return MobjectName;
}
