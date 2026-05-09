#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "UEMotionCamera.generated.h"

class ACineCameraActor;

UCLASS(BlueprintType)
class UUEMotionCamera : public UObject
{
	GENERATED_BODY()

public:
	void Init(ACineCameraActor* InCameraActor);

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Camera")
	void SetPosition(float X, float Y, float Z);

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Camera")
	FVector GetPosition() const;

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Camera")
	void SetRotation(float Pitch, float Yaw, float Roll);

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Camera")
	FRotator GetRotation() const;

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Camera")
	void SetFOV(float FOV);

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Camera")
	float GetFOV() const;

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Camera")
	void LookAt(const FVector& Target);

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Camera")
	void OrbitAround(const FVector& Center, float Radius, float AngleDegrees, float Height = 0.0f);

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Camera")
	void SetProjectionMode(int32 Mode);

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Camera")
	int32 GetProjectionMode() const;

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Camera")
	void SetOrthoWidth(float Width);

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Camera")
	float GetOrthoWidth() const;

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Camera")
	void SetOrthoNearClipPlane(float NearClip);

	UFUNCTION(BlueprintCallable, Category = "UEMotion|Camera")
	void SetOrthoFarClipPlane(float FarClip);

private:
	UPROPERTY()
	TWeakObjectPtr<ACineCameraActor> CameraActor;
};
