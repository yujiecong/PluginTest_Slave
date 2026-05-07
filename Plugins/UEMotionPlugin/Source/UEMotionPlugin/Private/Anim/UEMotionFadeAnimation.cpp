#include "UEMotionFadeAnimation.h"
#include "Core/UEMotionMobject.h"

void UUEMotionFadeAnimation::SetTargetMobject(UUEMotionMobject* InTarget)
{
	TargetMobject = InTarget;
}

void UUEMotionFadeAnimation::Reset()
{
	Super::Reset();
	if (TargetMobject.IsValid())
	{
		if (bFadeIn)
		{
			TargetMobject->SetOpacity(0.0f);
		}
		else
		{
			TargetMobject->SetOpacity(1.0f);
		}
	}
}

void UUEMotionFadeAnimation::TickAnimation(float DeltaTime, float EasedProgress)
{
	if (!TargetMobject.IsValid()) return;

	float Opacity;
	if (bFadeIn)
	{
		Opacity = EasedProgress;
	}
	else
	{
		Opacity = 1.0f - EasedProgress;
	}

	TargetMobject->SetOpacity(Opacity);
}
