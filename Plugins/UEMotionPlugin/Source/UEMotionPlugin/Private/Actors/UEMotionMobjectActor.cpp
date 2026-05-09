#include "UEMotionMobjectActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialParameterCollection.h"
#include "Kismet/KismetMaterialLibrary.h"

static const FString FadeMPCPath = TEXT("/Game/UEMotion/Materials/MPC_UEMotionFade");

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

	UWorld* World = GetWorld();
	if (World)
	{
		UMaterialParameterCollection* MPC = LoadObject<UMaterialParameterCollection>(nullptr, *FadeMPCPath);
		if (MPC)
		{
			UKismetMaterialLibrary::SetScalarParameterValue(World, MPC, FName("Opacity"), Opacity);
		}
	}

	UStaticMeshComponent* MeshComp = FindComponentByClass<UStaticMeshComponent>();
	if (MeshComp)
	{
		MeshComp->SetVisibility(Opacity > KINDA_SMALL_NUMBER);
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