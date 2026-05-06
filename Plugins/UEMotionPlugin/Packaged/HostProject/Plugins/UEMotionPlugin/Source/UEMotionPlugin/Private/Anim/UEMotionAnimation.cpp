#include "UEMotionAnimation.h"
#include "Utils/UEMotionBlueprintLibrary.h"

float UUEMotionAnimation::ApplyEasing(const FString& Type, float t)
{
	return UUEMotionBlueprintLibrary::ApplyEasingByName(Type, t);
}
