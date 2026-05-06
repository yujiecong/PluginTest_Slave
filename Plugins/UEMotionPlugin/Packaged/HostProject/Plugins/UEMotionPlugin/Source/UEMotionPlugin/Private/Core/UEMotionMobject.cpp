#include "UEMotionMobject.h"
#include "Actors/UEMotionSceneActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

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
		MaterialInstance->SetVectorParameterValue(TEXT("BaseColor"), Color);
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

FString UUEMotionMobject::GetName() const
{
	return MobjectName;
}
