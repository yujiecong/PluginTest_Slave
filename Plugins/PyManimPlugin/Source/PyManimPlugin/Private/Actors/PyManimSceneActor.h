#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UEMotionSceneActor.generated.h"

class UCameraComponent;

UCLASS()
class AUEMotionSceneActor : public AActor
{
	GENERATED_BODY()

public:
	AUEMotionSceneActor();

	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "UEMotion")
	UCameraComponent* GetCameraComponent() const;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USceneComponent* RootComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* CameraComp;
};
