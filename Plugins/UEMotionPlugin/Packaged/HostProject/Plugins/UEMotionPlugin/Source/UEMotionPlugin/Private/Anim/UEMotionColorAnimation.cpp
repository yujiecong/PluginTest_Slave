#include "UEMotionColorAnimation.h"
#include "Core/UEMotionMobject.h"

void UUEMotionColorAnimation::SetTargetMobject(UUEMotionMobject* InTarget)
{
	TargetMobject = InTarget;
	if (TargetMobject && !bStartSet)
	{
		StartColor = TargetMobject->GetColor();
	}
}

void UUEMotionColorAnimation::TickAnimation(float DeltaTime, float EasedProgress)
{
	if (!TargetMobject) return;
	FLinearColor Current = FLinearColor::LerpUsingHSV(StartColor, EndColor, EasedProgress);
	TargetMobject->SetColor(Current);
}
