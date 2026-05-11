#include "UEMotionMobjectActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

AUEMotionMobjectActor::AUEMotionMobjectActor()
{
	Opacity = 1.0f;
	CachedOpacity = 1.0f;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;
}

void AUEMotionMobjectActor::BeginPlay()
{
	Super::BeginPlay();
	CachedOpacity = Opacity;
}

void AUEMotionMobjectActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!FMath::IsNearlyEqual(Opacity, CachedOpacity))
	{
		CachedOpacity = Opacity;
		ApplyOpacityToMaterial();
	}
}

UStaticMeshComponent* AUEMotionMobjectActor::GetMeshComponent() const
{
	return FindComponentByClass<UStaticMeshComponent>();
}

void AUEMotionMobjectActor::EnsureDynamicMaterial()
{
	if (DynamicMaterial) return;

	UStaticMeshComponent* MeshComp = FindComponentByClass<UStaticMeshComponent>();
	if (!MeshComp) return;

	UMaterialInterface* CurrentMat = MeshComp->GetMaterial(0);
	if (!CurrentMat) return;

	DynamicMaterial = Cast<UMaterialInstanceDynamic>(CurrentMat);
	if (!DynamicMaterial)
	{
		DynamicMaterial = UMaterialInstanceDynamic::Create(CurrentMat, this);
		MeshComp->SetMaterial(0, DynamicMaterial);
	}
}

void AUEMotionMobjectActor::ApplyOpacityToMaterial()
{
	EnsureDynamicMaterial();
	if (DynamicMaterial)
	{
		DynamicMaterial->SetScalarParameterValue(FName("Opacity"), Opacity);
	}
}

void AUEMotionMobjectActor::SetOpacity(float InOpacity)
{
	Opacity = FMath::Clamp(InOpacity, 0.0f, 1.0f);
	CachedOpacity = Opacity;
	ApplyOpacityToMaterial();
}

#if WITH_EDITOR
void AUEMotionMobjectActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property)
	{
		FName PropertyName = PropertyChangedEvent.Property->GetFName();
		if (PropertyName == GET_MEMBER_NAME_CHECKED(AUEMotionMobjectActor, Opacity))
		{
			CachedOpacity = Opacity;
			ApplyOpacityToMaterial();
		}
	}
}
#endif