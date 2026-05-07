#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UEMotionSceneActor.generated.h"

class UUEMotionScene;
class UCameraComponent;

UCLASS()
class AUEMotionSceneActor : public AActor
{
	GENERATED_BODY()

public:
	AUEMotionSceneActor();

	virtual void Tick(float DeltaTime) override;

	void SetOwnerScene(UUEMotionScene* InScene);

	UFUNCTION(BlueprintCallable, Category = "UEMotion")
	UCameraComponent* GetCameraComponent() const { return CameraComponent; }

private:
	TWeakObjectPtr<UUEMotionScene> OwnerScene;

	UPROPERTY()
	UCameraComponent* CameraComponent;
};
