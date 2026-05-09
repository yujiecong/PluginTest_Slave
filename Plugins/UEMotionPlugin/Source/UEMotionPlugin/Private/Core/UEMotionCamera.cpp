#include "UEMotionCamera.h"
#include "CineCameraActor.h"
#include "Camera/CameraComponent.h"
#include "Kismet/KismetMathLibrary.h"

void UUEMotionCamera::Init(ACineCameraActor* InCameraActor)
{
	CameraActor = InCameraActor;
}

void UUEMotionCamera::SetPosition(float X, float Y, float Z)
{
	if (!CameraActor.IsValid()) return;
	CameraActor->SetActorLocation(FVector(X, Y, Z));
}

FVector UUEMotionCamera::GetPosition() const
{
	if (!CameraActor.IsValid()) return FVector::ZeroVector;
	return CameraActor->GetActorLocation();
}

void UUEMotionCamera::SetRotation(float Pitch, float Yaw, float Roll)
{
	if (!CameraActor.IsValid()) return;
	CameraActor->SetActorRotation(FRotator(Pitch, Yaw, Roll));
}

FRotator UUEMotionCamera::GetRotation() const
{
	if (!CameraActor.IsValid()) return FRotator::ZeroRotator;
	return CameraActor->GetActorRotation();
}

void UUEMotionCamera::SetFOV(float FOV)
{
	if (!CameraActor.IsValid()) return;
	UCameraComponent* CamComp = CameraActor->GetCameraComponent();
	if (CamComp)
	{
		CamComp->SetFieldOfView(FOV);
	}
}

float UUEMotionCamera::GetFOV() const
{
	if (!CameraActor.IsValid()) return 90.0f;
	UCameraComponent* CamComp = CameraActor->GetCameraComponent();
	if (CamComp)
	{
		return CamComp->FieldOfView;
	}
	return 90.0f;
}

void UUEMotionCamera::LookAt(const FVector& Target)
{
	if (!CameraActor.IsValid()) return;
	FVector Source = CameraActor->GetActorLocation();
	FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation(Source, Target);
	CameraActor->SetActorRotation(LookAtRot);
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

void UUEMotionCamera::SetProjectionMode(int32 Mode)
{
	if (!CameraActor.IsValid()) return;
	UCameraComponent* CamComp = CameraActor->GetCameraComponent();
	if (CamComp)
	{
		CamComp->SetProjectionMode(static_cast<ECameraProjectionMode::Type>(Mode));
	}
}

int32 UUEMotionCamera::GetProjectionMode() const
{
	if (!CameraActor.IsValid()) return 0;
	UCameraComponent* CamComp = CameraActor->GetCameraComponent();
	if (CamComp)
	{
		return static_cast<int32>(CamComp->ProjectionMode);
	}
	return 0;
}

void UUEMotionCamera::SetOrthoWidth(float Width)
{
	if (!CameraActor.IsValid()) return;
	UCameraComponent* CamComp = CameraActor->GetCameraComponent();
	if (CamComp)
	{
		CamComp->SetOrthoWidth(Width);
	}
}

float UUEMotionCamera::GetOrthoWidth() const
{
	if (!CameraActor.IsValid()) return 512.0f;
	UCameraComponent* CamComp = CameraActor->GetCameraComponent();
	if (CamComp)
	{
		return CamComp->OrthoWidth;
	}
	return 512.0f;
}

void UUEMotionCamera::SetOrthoNearClipPlane(float NearClip)
{
	if (!CameraActor.IsValid()) return;
	UCameraComponent* CamComp = CameraActor->GetCameraComponent();
	if (CamComp)
	{
		CamComp->SetOrthoNearClipPlane(NearClip);
	}
}

void UUEMotionCamera::SetOrthoFarClipPlane(float FarClip)
{
	if (!CameraActor.IsValid()) return;
	UCameraComponent* CamComp = CameraActor->GetCameraComponent();
	if (CamComp)
	{
		CamComp->SetOrthoFarClipPlane(FarClip);
	}
}
