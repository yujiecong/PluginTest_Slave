#include "UEMotionSceneActor.h"
#include "Core/UEMotionScene.h"
#include "Camera/CameraComponent.h"

AUEMotionSceneActor::AUEMotionSceneActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(RootComponent);
	CameraComponent->SetFieldOfView(90.0f);
}

void AUEMotionSceneActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (OwnerScene.IsValid())
	{
		OwnerScene->Tick(DeltaTime);
	}
}

void AUEMotionSceneActor::SetOwnerScene(UUEMotionScene* InScene)
{
	OwnerScene = InScene;
}
