#include "Anim/UEMotionTransformAnimation.h"
#include "Core/UEMotionMobject.h"

void UUEMotionTransformAnimation::SetSourceMobject(UUEMotionMobject* Source)
{
	SourceMobject = Source;
}

void UUEMotionTransformAnimation::SetTargetMobject(UUEMotionMobject* Target)
{
	TargetMobject = Target;
}

void UUEMotionTransformAnimation::Reset()
{
	Super::Reset();
	MorphProgress = 0.0f;
}

void UUEMotionTransformAnimation::UpdateAnimation(float DeltaTime, float EasedProgress)
{
	MorphProgress = FMath::Clamp(EasedProgress, 0.0f, 1.0f);
}
