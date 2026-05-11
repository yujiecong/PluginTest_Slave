#pragma once

#include "CoreMinimal.h"
#include "CineCameraActor.h"
#include "UEMotionSceneActor.generated.h"

class UUEMotionScene;

UCLASS()
class AUEMotionSceneActor : public ACineCameraActor
{
	GENERATED_BODY()

public:
	AUEMotionSceneActor(const FObjectInitializer& ObjectInitializer);

	void SetOwnerScene(UUEMotionScene* InScene);

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Camera")
	void ApplyCameraAspectRatio();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UEMotion|Camera", meta = (ClampMin = 0.1, UIMin = 0.1))
	float CameraAspectRatio;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UEMotion|Camera", meta = (ClampMin = 1.0, UIMin = 1.0))
	float CameraSensorWidth;

protected:
	virtual void Tick(float DeltaTime) override;

private:
	TWeakObjectPtr<UUEMotionScene> OwnerScene;
};
