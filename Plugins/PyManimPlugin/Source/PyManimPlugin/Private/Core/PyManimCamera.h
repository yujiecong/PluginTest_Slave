#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PyManimCamera.generated.h"

class APyManimSceneActor;
class UCameraComponent;

UCLASS(BlueprintType)
class UPyManimCamera : public UObject
{
	GENERATED_BODY()

public:
	void Init(APyManimSceneActor* InSceneActor);

	UFUNCTION(BlueprintCallable, Category = "PyManim|Camera")
	void SetPosition(float X, float Y, float Z);

	UFUNCTION(BlueprintCallable, Category = "PyManim|Camera")
	FVector GetPosition() const;

	UFUNCTION(BlueprintCallable, Category = "PyManim|Camera")
	void SetRotation(float Pitch, float Yaw, float Roll);

	UFUNCTION(BlueprintCallable, Category = "PyManim|Camera")
	FRotator GetRotation() const;

	UFUNCTION(BlueprintCallable, Category = "PyManim|Camera")
	void SetFOV(float FOV);

	UFUNCTION(BlueprintCallable, Category = "PyManim|Camera")
	float GetFOV() const;

	UFUNCTION(BlueprintCallable, Category = "PyManim|Camera")
	void LookAt(const FVector& Target);

	UFUNCTION(BlueprintCallable, Category = "PyManim|Camera")
	void OrbitAround(const FVector& Center, float Radius, float AngleDegrees, float Height = 0.0f);

private:
	UPROPERTY()
	TWeakObjectPtr<APyManimSceneActor> SceneActor;
};
