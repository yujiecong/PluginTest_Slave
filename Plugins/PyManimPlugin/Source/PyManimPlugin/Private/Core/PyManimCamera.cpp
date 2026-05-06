#include "UEMotionCamera.h"
#include "Actors/UEMotionSceneActor.h"
#include "Camera/CameraComponent.h"
#include "Kismet/KismetMathLibrary.h"

void UUEMotionCamera::Init(AUEMotionSceneActor* InSceneActor)
{
	SceneActor = InSceneActor;
}

void UUEMotionCamera::SetPosition(float X, float Y, float Z)
{
	if (!SceneActor.IsValid()) return;
	SceneActor->SetActorLocation(FVector(X, Y, Z));
}

FVector UUEMotionCamera::GetPosition() const
{
	if (!SceneActor.IsValid()) return FVector::ZeroVector;
	return SceneActor->GetActorLocation();
}

void UUEMotionCamera::SetRotation(float Pitch, float Yaw, float Roll)
{
	if (!SceneActor.IsValid()) return;
	SceneActor->SetActorRotation(FRotator(Pitch, Yaw, Roll));
}

FRotator UUEMotionCamera::GetRotation() const
{
	if (!SceneActor.IsValid()) return FRotator::ZeroRotator;
	return SceneActor->GetActorRotation();
}

void UUEMotionCamera::SetFOV(float FOV)
{
	if (!SceneActor.IsValid()) return;
	UCameraComponent* CamComp = SceneActor->GetCameraComponent();
	if (CamComp)
	{
		CamComp->SetFieldOfView(FOV);
	}
}

float UUEMotionCamera::GetFOV() const
{
	if (!SceneActor.IsValid()) return 90.0f;
	UCameraComponent* CamComp = SceneActor->GetCameraComponent();
	if (CamComp)
	{
		return CamComp->FieldOfView;
	}
	return 90.0f;
}

void UUEMotionCamera::LookAt(const FVector& Target)
{
	if (!SceneActor.IsValid()) return;
	FVector Source = SceneActor->GetActorLocation();
	FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation(Source, Target);
	SceneActor->SetActorRotation(LookAtRot);
}

void UUEMotionCamera::OrbitAround(const FVector& Center, float Radius, float AngleDegrees, float Height)
{
	float AngleRad = FMath::DegreesToRadians(AngleDegrees);
	float X = Center.X + Radius * FMath::Cos(AngleRad);
	float Y = Center.Y + Radius * FMath::Sin(AngleRad);
	float Z = Center.Z + Height;

	SetPosition(X, Y, Z);
	LookAt(Center);
}
