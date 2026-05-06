#include "PyManimColorAnimation.h"
#include "Core/PyManimMobject.h"

void UPyManimColorAnimation::SetTargetMobject(UPyManimMobject* InTarget)
{
	TargetMobject = InTarget;
	if (TargetMobject && !bStartSet)
	{
		StartColor = TargetMobject->GetColor();
	}
}

void UPyManimColorAnimation::TickAnimation(float DeltaTime, float EasedProgress)
{
	if (!TargetMobject) return;
	FLinearColor Current = FLinearColor::LerpUsingHSV(StartColor, EndColor, EasedProgress);
	TargetMobject->SetColor(Current);
}
