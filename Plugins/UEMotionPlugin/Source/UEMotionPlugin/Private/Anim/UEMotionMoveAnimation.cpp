#include "UEMotionMoveAnimation.h"
#include "Core/UEMotionMobject.h"

void UUEMotionMoveAnimation::SetTargetMobject(UUEMotionMobject* InTarget)
{
	TargetMobject = InTarget;
	if (TargetMobject && !bStartSet)
	{
		Start = TargetMobject->GetLocation();
	}
}

void UUEMotionMoveAnimation::UpdateAnimation(float DeltaTime, float EasedProgress)
{
	if (!TargetMobject) return;
	FVector Current = FMath::Lerp(Start, End, EasedProgress);
	TargetMobject->SetLocation(Current);
}
