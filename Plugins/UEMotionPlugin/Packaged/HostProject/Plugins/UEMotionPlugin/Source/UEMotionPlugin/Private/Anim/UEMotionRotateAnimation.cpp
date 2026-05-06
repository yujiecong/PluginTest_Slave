#include "UEMotionRotateAnimation.h"
#include "Core/UEMotionMobject.h"

void UUEMotionRotateAnimation::SetTargetMobject(UUEMotionMobject* InTarget)
{
	TargetMobject = InTarget;
	if (TargetMobject)
	{
		StartRotation = TargetMobject->GetRotation();
	}
}

void UUEMotionRotateAnimation::TickAnimation(float DeltaTime, float EasedProgress)
{
	if (!TargetMobject) return;

	float CurrentAngle = TargetAngle * EasedProgress;
	FRotator DeltaRotation = FRotator(
		Axis.X * CurrentAngle,
		Axis.Y * CurrentAngle,
		Axis.Z * CurrentAngle
	);
	TargetMobject->SetRotation(StartRotation + DeltaRotation);
}
