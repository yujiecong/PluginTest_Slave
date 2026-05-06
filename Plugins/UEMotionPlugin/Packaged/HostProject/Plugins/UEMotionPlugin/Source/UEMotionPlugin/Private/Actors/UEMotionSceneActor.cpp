#include "UEMotionSceneActor.h"
#include "Components/SceneComponent.h"
#include "Camera/CameraComponent.h"

AUEMotionSceneActor::AUEMotionSceneActor()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(RootComp);

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CameraComp->SetupAttachment(RootComp);
}

void AUEMotionSceneActor::BeginPlay()
{
	Super::BeginPlay();
}

void AUEMotionSceneActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

UCameraComponent* AUEMotionSceneActor::GetCameraComponent() const
{
	return CameraComp;
}
