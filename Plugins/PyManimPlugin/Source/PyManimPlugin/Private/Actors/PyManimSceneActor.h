#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PyManimSceneActor.generated.h"

class UCameraComponent;

UCLASS()
class APyManimSceneActor : public AActor
{
	GENERATED_BODY()

public:
	APyManimSceneActor();

	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "PyManim")
	UCameraComponent* GetCameraComponent() const;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USceneComponent* RootComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* CameraComp;
};
