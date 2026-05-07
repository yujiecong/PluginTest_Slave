#include "UEMotionRotateAnimation.h"
#include "Core/UEMotionMobject.h"

void UUEMotionRotateAnimation::SetTargetMobject(UUEMotionMobject* InTarget)
{
	TargetMobject = InTarget;
	if (TargetMobject)
	{
		StartQuat = TargetMobject->GetRotation().Quaternion();
	}
}

void UUEMotionRotateAnimation::TickAnimation(float DeltaTime, float EasedProgress)
{
	if (!TargetMobject) return;

	float CurrentAngle = TargetAngle * EasedProgress;
	FVector NormalizedAxis = Axis.GetSafeNormal();
	if (NormalizedAxis.IsNearlyZero())
	{
		NormalizedAxis = FVector::UpVector;
	}

	FQuat DeltaQuat = FQuat(NormalizedAxis, FMath::DegreesToRadians(CurrentAngle));
	FQuat ResultQuat = DeltaQuat * StartQuat;
	TargetMobject->SetRotation(ResultQuat.Rotator());
}
