#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UEMotionSceneActor.generated.h"

class UUEMotionScene;

UCLASS()
class AUEMotionSceneActor : public AActor
{
	GENERATED_BODY()

public:
	AUEMotionSceneActor();

	virtual void Tick(float DeltaTime) override;

	void SetOwnerScene(UUEMotionScene* InScene);

private:
	TWeakObjectPtr<UUEMotionScene> OwnerScene;
};
