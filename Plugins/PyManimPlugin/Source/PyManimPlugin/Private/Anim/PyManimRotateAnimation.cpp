#include "PyManimRotateAnimation.h"
#include "Core/PyManimMobject.h"

void UPyManimRotateAnimation::SetTargetMobject(UPyManimMobject* InTarget)
{
	TargetMobject = InTarget;
	if (TargetMobject)
	{
		StartRotation = TargetMobject->GetRotation();
	}
}

void UPyManimRotateAnimation::TickAnimation(float DeltaTime, float EasedProgress)
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
