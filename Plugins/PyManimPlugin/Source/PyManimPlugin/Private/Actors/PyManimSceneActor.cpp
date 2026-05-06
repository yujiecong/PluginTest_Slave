#include "PyManimSceneActor.h"
#include "Components/SceneComponent.h"
#include "Camera/CameraComponent.h"

APyManimSceneActor::APyManimSceneActor()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(RootComp);

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CameraComp->SetupAttachment(RootComp);
}

void APyManimSceneActor::BeginPlay()
{
	Super::BeginPlay();
}

void APyManimSceneActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

UCameraComponent* APyManimSceneActor::GetCameraComponent() const
{
	return CameraComp;
}
