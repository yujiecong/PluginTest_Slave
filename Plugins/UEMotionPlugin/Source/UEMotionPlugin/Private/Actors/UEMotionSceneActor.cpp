#include "UEMotionSceneActor.h"
#include "Core/UEMotionScene.h"

AUEMotionSceneActor::AUEMotionSceneActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
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
