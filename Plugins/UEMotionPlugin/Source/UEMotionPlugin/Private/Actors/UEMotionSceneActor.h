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

	virtual void Tick(float DeltaTime) override;

	void SetOwnerScene(UUEMotionScene* InScene);

private:
	TWeakObjectPtr<UUEMotionScene> OwnerScene;
};
