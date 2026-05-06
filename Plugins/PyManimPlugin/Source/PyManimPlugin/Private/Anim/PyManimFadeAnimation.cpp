#include "UEMotionFadeAnimation.h"
#include "Core/UEMotionMobject.h"

void UUEMotionFadeAnimation::SetTargetMobject(UUEMotionMobject* InTarget)
{
	TargetMobject = InTarget;
}

void UUEMotionFadeAnimation::TickAnimation(float DeltaTime, float EasedProgress)
{
	if (!TargetMobject) return;

	if (bFadeOut)
	{
		TargetMobject->SetVisibility(EasedProgress < 0.99f);
	}
	else
	{
		TargetMobject->SetVisibility(EasedProgress > 0.01f);
	}
}
