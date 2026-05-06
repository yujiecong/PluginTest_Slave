#include "PyManimMoveAnimation.h"
#include "Core/PyManimMobject.h"

void UPyManimMoveAnimation::SetTargetMobject(UPyManimMobject* InTarget)
{
	TargetMobject = InTarget;
	if (TargetMobject && !bStartSet)
	{
		Start = TargetMobject->GetLocation();
	}
}

void UPyManimMoveAnimation::TickAnimation(float DeltaTime, float EasedProgress)
{
	if (!TargetMobject) return;
	FVector Current = FMath::Lerp(Start, End, EasedProgress);
	TargetMobject->SetLocation(Current);
}
