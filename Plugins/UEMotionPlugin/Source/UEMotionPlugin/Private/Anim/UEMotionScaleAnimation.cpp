#include "UEMotionScaleAnimation.h"
#include "Core/UEMotionMobject.h"

void UUEMotionScaleAnimation::SetTargetMobject(UUEMotionMobject* InTarget)
{
	TargetMobject = InTarget;
	if (TargetMobject && !bStartSet)
	{
		StartScale = TargetMobject->GetScale();
	}
}

void UUEMotionScaleAnimation::TickAnimation(float DeltaTime, float EasedProgress)
{
	if (!TargetMobject) return;
	FVector Current = FMath::Lerp(StartScale, EndScale, EasedProgress);
	TargetMobject->SetScale(Current);
}
