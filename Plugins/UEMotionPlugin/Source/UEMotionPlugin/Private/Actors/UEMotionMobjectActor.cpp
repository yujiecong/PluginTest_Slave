#include "UEMotionMobjectActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

AUEMotionMobjectActor::AUEMotionMobjectActor()
{
	Opacity = 1.0f;
}

void AUEMotionMobjectActor::BeginPlay()
{
	Super::BeginPlay();
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

void AUEMotionMobjectActor::SetOpacity(float InOpacity)
{
	Opacity = FMath::Clamp(InOpacity, 0.0f, 1.0f);

	EnsureDynamicMaterial();
	if (DynamicMaterial)
	{
		DynamicMaterial->SetScalarParameterValue(FName("Opacity"), Opacity);
	}
}

#if WITH_EDITOR
void AUEMotionMobjectActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName PropertyName = PropertyChangedEvent.GetPropertyName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(AUEMotionMobjectActor, Opacity))
	{
		SetOpacity(Opacity);
	}
}
#endif