#include "UEMotionSceneActor.h"
#include "Core/UEMotionScene.h"
#include "CineCameraComponent.h"

AUEMotionSceneActor::AUEMotionSceneActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CameraAspectRatio = 1.0f;
	CameraSensorWidth = 24.0f;

	ApplyCameraAspectRatio();
}

void AUEMotionSceneActor::SetOwnerScene(UUEMotionScene* InScene)
{
	OwnerScene = InScene;
}

void AUEMotionSceneActor::ApplyCameraAspectRatio()
{
	UCineCameraComponent* CineComp = Cast<UCineCameraComponent>(GetCameraComponent());
	if (CineComp)
	{
		float SafeRatio = FMath::Max(CameraAspectRatio, 0.1f);
		float SafeWidth = FMath::Max(CameraSensorWidth, 1.0f);
		CineComp->Filmback.SensorWidth = SafeWidth;
		CineComp->Filmback.SensorHeight = SafeWidth / SafeRatio;
	}
}
