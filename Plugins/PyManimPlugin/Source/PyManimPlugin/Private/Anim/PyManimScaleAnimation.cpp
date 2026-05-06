#include "PyManimScaleAnimation.h"
#include "Core/PyManimMobject.h"

void UPyManimScaleAnimation::SetTargetMobject(UPyManimMobject* InTarget)
{
	TargetMobject = InTarget;
	if (TargetMobject && !bStartSet)
	{
		StartScale = TargetMobject->GetScale();
	}
}

void UPyManimScaleAnimation::TickAnimation(float DeltaTime, float EasedProgress)
{
	if (!TargetMobject) return;
	FVector Current = FMath::Lerp(StartScale, EndScale, EasedProgress);
	TargetMobject->SetScale(Current);
}
