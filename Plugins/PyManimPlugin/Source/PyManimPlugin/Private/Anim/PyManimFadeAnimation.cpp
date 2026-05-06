#include "PyManimFadeAnimation.h"
#include "Core/PyManimMobject.h"

void UPyManimFadeAnimation::SetTargetMobject(UPyManimMobject* InTarget)
{
	TargetMobject = InTarget;
}

void UPyManimFadeAnimation::TickAnimation(float DeltaTime, float EasedProgress)
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
